#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
PVE VM Wake-on-LAN listener (Python)

参考：https://forum.proxmox.com/threads/wake-on-lan-for-vm.84355/

功能：
- 监听 UDP 端口 9/7 的 WOL 魔术包（broadcast/unicast 均可）
- 从 /etc/pve/qemu-server/*.conf 解析 VM 的网卡 MAC，建立 MAC→VMID 映射
- 收到魔术包后匹配对应 VM，执行 `qm start <vmid>` 启动
- 支持可选映射文件（JSON/YAML），以及去抖（短时间重复包只触发一次）

使用：
  python3 tools/pve_wol.py --bind 0.0.0.0 --port 9 --verbose

可选：
  - 以 systemd 方式常驻（见本文件底部说明）

需要在 Proxmox 主机上运行（具备 qm 命令）。
"""

import argparse
import json
import os
import re
import select
import socket
import struct
import subprocess
import sys
import threading
import time
import shutil
from pathlib import Path

try:
    import yaml  # 可选
except Exception:  # noqa: BLE001
    yaml = None

MAC_RE = re.compile(r"^([0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}$")
PVE_QEMU_CONF_DIR = "/etc/pve/qemu-server"
DEFAULT_INSTALL_PATH = "/usr/local/bin/pve-wol"
DEFAULT_SERVICE_PATH = "/etc/systemd/system/pve-wol.service"


def log(msg: str):
    ts = time.strftime("%Y-%m-%d %H:%M:%S")
    print(f"[{ts}] {msg}")


def dlog(enabled: bool, msg: str):
    if enabled:
        log(msg)


def norm_mac(mac: str) -> str:
    mac = mac.strip().lower()
    mac = mac.replace("-", ":")
    # 去掉常见前缀 0x
    mac = mac.replace("0x", "")
    # 如果是无分隔 12 hex，格式化为 xx:xx:xx:xx:xx:xx
    if re.fullmatch(r"[0-9a-f]{12}", mac):
        mac = ":".join(mac[i:i+2] for i in range(0, 12, 2))
    if not MAC_RE.match(mac):
        raise ValueError(f"非法 MAC: {mac}")
    return mac


def parse_magic_packet(data: bytes):
    """解析 WOL 魔术包，返回目标 MAC（str）或 None。
    魔术包格式：6 字节 0xFF + 16 次 MAC 重复 [+ 可选 4~6 字节密码]
    """
    if len(data) < 6 + 16 * 6:
        return None
    if data[:6] != b"\xff" * 6:
        return None
    # 读取第一个 MAC
    mac_bytes = data[6:12]
    # 验证后续 15 次重复
    for i in range(1, 16):
        if data[6 + i * 6:12 + i * 6] != mac_bytes:
            return None
    mac = ":".join(f"{b:02x}" for b in mac_bytes)
    return mac


def load_map_from_file(path: Path, verbose: bool):
    if not path.exists():
        raise FileNotFoundError(str(path))
    if path.suffix.lower() in {".yml", ".yaml"}:
        if yaml is None:
            raise RuntimeError("未安装 pyyaml，无法读取 YAML。请使用 JSON 或安装：pip install pyyaml")
        with path.open("r", encoding="utf-8") as f:
            data = yaml.safe_load(f) or {}
    else:
        with path.open("r", encoding="utf-8") as f:
            data = json.load(f)
    mapping = {}
    for k, v in (data or {}).items():
        try:
            mapping[norm_mac(k)] = int(v)
        except Exception as e:  # noqa: BLE001
            dlog(verbose, f"忽略映射项 {k} -> {v}: {e}")
    return mapping


def scan_pve_vm_macs(verbose: bool):
    """扫描 /etc/pve/qemu-server/*.conf，提取 netX 的 MAC，返回 {mac: vmid}。"""
    mapping = {}
    conf_dir = Path(PVE_QEMU_CONF_DIR)
    if not conf_dir.exists():
        dlog(verbose, f"目录不存在：{conf_dir}，跳过自动扫描")
        return mapping
    for cfg in sorted(conf_dir.glob("*.conf")):
        vmid = cfg.stem
        try:
            vmid_i = int(vmid)
        except Exception:
            continue
        try:
            text = cfg.read_text(encoding="utf-8", errors="ignore")
        except Exception as e:  # noqa: BLE001
            dlog(verbose, f"读取失败 {cfg}: {e}")
            continue
        # 匹配类似：net0: virtio=AA:BB:CC:DD:EE:FF,... 或 e1000=...
        for line in text.splitlines():
            if not line.startswith("net"):
                continue
            # 提取 MAC
            m = re.search(r"(?:virtio|e1000|rtl8139|vmxnet3|ne2k_pci)\s*=\s*([0-9A-Fa-f:]{17})", line)
            if not m:
                m = re.search(r"macaddr=([0-9A-Fa-f:]{17})", line)
            if m:
                try:
                    mac = norm_mac(m.group(1))
                    mapping[mac] = vmid_i
                except Exception as e:  # noqa: BLE001
                    dlog(verbose, f"忽略行 `{line}`: {e}")
    return mapping


def build_mapping(args) -> dict:
    mapping = {}
    if args.map and Path(args.map).exists():
        mapping.update(load_map_from_file(Path(args.map), args.verbose))
    auto = scan_pve_vm_macs(args.verbose)
    # 配置文件优先，自动扫描补充缺失
    for mac, vmid in auto.items():
        mapping.setdefault(mac, vmid)
    return mapping


def qm_start(vmid: int, dry_run: bool, verbose: bool) -> bool:
    cmd = ["/usr/sbin/qm", "start", str(vmid)] if os.path.exists("/usr/sbin/qm") else ["qm", "start", str(vmid)]
    dlog(verbose, f"执行: {' '.join(cmd)}")
    if dry_run:
        return True
    try:
        res = subprocess.run(cmd, check=False, capture_output=True, text=True)
        if res.returncode == 0:
            dlog(verbose, f"qm start {vmid} 成功: {res.stdout.strip()}")
            return True
        else:
            log(f"qm start {vmid} 失败: rc={res.returncode} stderr={res.stderr.strip()}")
            return False
    except Exception as e:  # noqa: BLE001
        log(f"执行 qm 失败: {e}")
        return False


def listen_and_serve(args):
    mapping = build_mapping(args)
    if not mapping:
        log("未发现任何 VM MAC→VMID 映射。请检查 /etc/pve/qemu-server/ 或通过 --map 指定。")
    else:
        dlog(args.verbose, f"映射表: {json.dumps(mapping, indent=2, ensure_ascii=False)}")

    # 去抖：同一 VM 的触发最小间隔（秒）
    debounce = {}
    debounce_window = args.debounce

    # 支持同时监听多个端口
    ports = sorted(set([int(p) for p in args.port]))

    socks = []
    for port in ports:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        try:
            s.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        except Exception:
            pass
        s.bind((args.bind, port))
        socks.append(s)
        dlog(args.verbose, f"监听 UDP {args.bind}:{port}")

    try:
        while True:
            rlist, _, _ = select.select(socks, [], [], 1.0)
            if not rlist:
                continue
            for s in rlist:
                try:
                    data, addr = s.recvfrom(2048)
                except Exception:
                    continue
                mac = parse_magic_packet(data)
                if not mac:
                    dlog(args.verbose, f"非魔术包（忽略） 来自 {addr}")
                    continue
                dlog(args.verbose, f"收到 WOL 包: MAC={mac} 来自 {addr}")
                mac_n = norm_mac(mac)
                vmid = mapping.get(mac_n)
                if vmid is None:
                    log(f"未找到匹配 VM：MAC={mac_n}，忽略。")
                    continue
                now = time.time()
                last = debounce.get(vmid, 0)
                if now - last < debounce_window:
                    dlog(args.verbose, f"VM {vmid} 去抖：{now-last:.2f}s < {debounce_window}s")
                    continue
                if qm_start(vmid, args.dry_run, args.verbose):
                    debounce[vmid] = now
    finally:
        for s in socks:
            try:
                s.close()
            except Exception:
                pass


def _require_root():
    if os.geteuid() != 0:
        raise SystemExit("需要 root 权限执行该操作（安装/卸载）。")


def _run(cmd: list[str]) -> tuple[int, str, str]:
    res = subprocess.run(cmd, check=False, capture_output=True, text=True)
    return res.returncode, res.stdout, res.stderr


def _render_unit(exec_path: str, bind: str, ports: list[int]) -> str:
    ports_args = " ".join(f"--port {p}" for p in ports)
    return f"""
[Unit]
Description=PVE VM Wake-on-LAN listener
After=network-online.target
Wants=network-online.target

[Service]
Type=simple
User=root
Group=root
ExecStart=/usr/bin/python3 {exec_path} --bind {bind} {ports_args} -v
Restart=always
RestartSec=2

[Install]
WantedBy=multi-user.target
""".lstrip()


def cmd_install(args):
    _require_root()
    src = os.path.abspath(sys.argv[0])
    dst = args.path or DEFAULT_INSTALL_PATH
    svc = args.service or DEFAULT_SERVICE_PATH
    bind = args.bind
    ports = sorted(set(int(p) for p in (args.port or ["9", "7"])) )

    os.makedirs(os.path.dirname(dst), exist_ok=True)
    shutil.copy2(src, dst)
    os.chmod(dst, 0o755)

    unit = _render_unit(dst, bind, ports)
    Path(svc).write_text(unit, encoding="utf-8")

    _run(["systemctl", "daemon-reload"]) 
    if not args.no_enable:
        _run(["systemctl", "enable", "--now", "pve-wol"])  # 以单元名操作
    log(f"已安装：二进制 {dst}，systemd 单元 {svc}")


def cmd_uninstall(args):
    _require_root()
    dst = args.path or DEFAULT_INSTALL_PATH
    svc = args.service or DEFAULT_SERVICE_PATH

    # 停止/禁用服务
    _run(["systemctl", "disable", "--now", "pve-wol"]) 

    # 删除单元
    try:
        if Path(svc).exists():
            Path(svc).unlink()
    except Exception as e:
        log(f"删除单元文件失败：{e}")
    _run(["systemctl", "daemon-reload"]) 

    # 删除安装文件
    try:
        if Path(dst).exists():
            Path(dst).unlink()
    except Exception as e:
        log(f"删除安装文件失败：{e}")
    log("已卸载 pve-wol（如需彻底清理日志可手动清理 journal）")


def main():
    parser = argparse.ArgumentParser(description="Proxmox PVE VM Wake-on-LAN Listener")
    sub = parser.add_subparsers(dest="cmd")

    # 运行子命令（默认）
    runp = sub.add_parser("run", help="以监听模式运行（默认）")
    runp.add_argument("--bind", default="0.0.0.0", help="监听地址，默认 0.0.0.0")
    runp.add_argument("--port", action="append", default=["9", "7"], help="监听端口，可多次指定，默认 9 和 7")
    runp.add_argument("--map", help="可选：指定 JSON/YAML 的 MAC→VMID 映射文件")
    runp.add_argument("--debounce", type=float, default=5.0, help="同一 VM 去抖间隔秒，默认 5s")
    runp.add_argument("--dry-run", action="store_true", help="仅打印将要执行的动作，不实际启动 VM")
    runp.add_argument("--verbose", "-v", action="store_true", help="详细日志")

    # 安装子命令
    inst = sub.add_parser("install", help="安装为系统服务（复制到 /usr/local/bin 并创建 systemd 单元）")
    inst.add_argument("--path", help=f"安装目标路径（默认 {DEFAULT_INSTALL_PATH}）")
    inst.add_argument("--service", help=f"systemd 单元路径（默认 {DEFAULT_SERVICE_PATH}）")
    inst.add_argument("--bind", default="0.0.0.0", help="服务监听地址")
    inst.add_argument("--port", action="append", default=["9", "7"], help="服务监听端口，可多次指定")
    inst.add_argument("--no-enable", action="store_true", help="仅安装/生成单元，不启用/启动")

    # 卸载子命令
    unin = sub.add_parser("uninstall", help="卸载系统服务并删除已安装文件")
    unin.add_argument("--path", help=f"安装目标路径（默认 {DEFAULT_INSTALL_PATH}）")
    unin.add_argument("--service", help=f"systemd 单元路径（默认 {DEFAULT_SERVICE_PATH}）")

    # 兼容无子命令直接运行
    if len(sys.argv) == 1 or (len(sys.argv) > 1 and not sys.argv[1].startswith("install") and not sys.argv[1].startswith("uninstall")):
        # 将无子命令视为 run
        argv = ["run", *sys.argv[1:]]
        args = parser.parse_args(argv)
    else:
        args = parser.parse_args()

    if args.cmd == "install":
        cmd_install(args)
        return
    if args.cmd == "uninstall":
        cmd_uninstall(args)
        return

    # 简单权限检查（run 模式）
    if os.geteuid() != 0:
        log("警告：建议以 root 运行以确保有权限执行 qm 与绑定端口。")

    try:
        listen_and_serve(args)
    except KeyboardInterrupt:
        log("收到中断，退出。")


if __name__ == "__main__":
    main()

"""
Systemd 示例：/etc/systemd/system/pve-wol.service
-----------------------------------------------
[Unit]
Description=PVE VM Wake-on-LAN listener
After=network-online.target
Wants=network-online.target

[Service]
Type=simple
User=root
Group=root
ExecStart=/usr/bin/python3 /usr/local/bin/pve-wol --bind 0.0.0.0 --port 9 --port 7 -v
Restart=always
RestartSec=2

[Install]
WantedBy=multi-user.target

操作：
  systemctl daemon-reload
  systemctl enable --now pve-wol
日志：
  journalctl -u pve-wol -f

自定义映射文件（可选）：/etc/pve-wol-map.yaml
----------------------------------------------
# 内容示例（MAC 必须与 VM 配置中的网卡 MAC 一致）
"52:54:00:12:34:56": 100
"9e:fa:01:02:03:04": 101

脚本会先加载映射文件（如提供），再自动扫描 /etc/pve/qemu-server/*.conf 补齐。
"""
