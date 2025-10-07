#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Audit ARB localization keys that are not referenced in Dart code.

Usage:
  python3 scripts/l10n_audit.py \
    --arb-dir app/lib/l10n \
    --lib-dir app/lib \
    [--report-json /path/to/report.json]

Notes:
- Keys starting with '@' and the special key '@@locale' are ignored.
- Dart usage patterns supported:
  - l10n.someKey
  - AppLocalizations.of(context).someKey
- The generated code directory 'app/lib/l10n/gen' is ignored by default.
- Exit code: 0 if no unused keys; 1 if there are unused keys; 2 on error.
"""
from __future__ import annotations
import argparse
import json
import re
import sys
from pathlib import Path
from typing import Dict, List, Set, Tuple

L10N_ALIAS_REGEX = re.compile(r"\bl10n\.(?P<key>[A-Za-z0-9_]+)\b")
# 允许 of(...) 与 .key 之间存在任意空白/换行
APPLOC_REGEX = re.compile(r"\bAppLocalizations\.of\([^\)]*\)\s*\.(?P<key>[A-Za-z0-9_]+)\b", re.MULTILINE)

DEFAULT_ARB_DIR = Path("app/lib/l10n")
DEFAULT_LIB_DIR = Path("app/lib")
EXCLUDE_DIRS = {"app/lib/l10n/gen"}


def _load_json_lenient(text: str) -> Dict:
    # 1) 移除仅包含逗号的行（项目中常见风格）
    text = re.sub(r"^\s*,\s*$", "", text, flags=re.MULTILINE)
    # 2) 允许尾随逗号：..., } / ..., ]
    text = re.sub(r",\s*([}\]])", r"\1", text)
    return json.loads(text)


TOP_KEY_REGEX = re.compile(r'"(?P<key>[A-Za-z0-9_@]+)"\s*:\s*')


def read_arb_keys(arb_path: Path) -> Set[str]:
    """Extract only top-level keys from ARB text without strict JSON parsing.
    We track brace depth outside of strings and only capture keys when depth==1.
    Ignore @@locale and any @-prefixed metadata keys.
    """
    s = arb_path.read_text(encoding="utf-8", errors="ignore")
    keys: Set[str] = set()
    depth = 0
    i = 0
    in_str = False
    esc = False
    while i < len(s):
        ch = s[i]
        if in_str:
            if esc:
                esc = False
            elif ch == '\\':
                esc = True
            elif ch == '"':
                in_str = False
            i += 1
            continue
        else:
            if ch == '"':
                # possible key start only when depth==1 and followed by key pattern and ':'
                if depth == 1:
                    m = TOP_KEY_REGEX.match(s, i)
                    if m:
                        k = m.group('key')
                        if k != '@@locale' and not k.startswith('@'):
                            keys.add(k)
                        # move to after this match to continue scanning
                        i = m.end()
                        continue
                in_str = True
            elif ch == '{':
                depth += 1
            elif ch == '}':
                depth = max(0, depth - 1)
        i += 1
    return keys


def enumerate_arb_files(arb_dir: Path) -> List[Path]:
    return sorted(p for p in arb_dir.glob("*.arb") if p.is_file())


def is_excluded(path: Path) -> bool:
    p = str(path.as_posix())
    return any(p.startswith(ex) for ex in EXCLUDE_DIRS)


def collect_used_keys(lib_dir: Path) -> Set[str]:
    used: Set[str] = set()
    for path in lib_dir.rglob("*.dart"):
        if not path.is_file():
            continue
        if is_excluded(path):
            continue
        try:
            text = path.read_text(encoding="utf-8", errors="ignore")
        except Exception:
            continue
        for m in L10N_ALIAS_REGEX.finditer(text):
            used.add(m.group("key"))
        for m in APPLOC_REGEX.finditer(text):
            used.add(m.group("key"))
    return used


def main() -> int:
    parser = argparse.ArgumentParser(description="Audit unused ARB localization keys")
    parser.add_argument("--arb-dir", type=Path, default=DEFAULT_ARB_DIR)
    parser.add_argument("--lib-dir", type=Path, default=DEFAULT_LIB_DIR)
    parser.add_argument("--report-json", type=Path, default=None)
    args = parser.parse_args()

    if not args.arb_dir.exists():
        print(f"[ERROR] ARB directory not found: {args.arb_dir}", file=sys.stderr)
        return 2
    if not args.lib_dir.exists():
        print(f"[ERROR] Lib directory not found: {args.lib_dir}", file=sys.stderr)
        return 2

    arb_files = enumerate_arb_files(args.arb_dir)
    if not arb_files:
        print(f"[ERROR] No .arb files found in: {args.arb_dir}", file=sys.stderr)
        return 2

    used_keys = collect_used_keys(args.lib_dir)

    report: Dict[str, Dict[str, List[str]]] = {}
    total_unused = 0

    for arb in arb_files:
        keys = read_arb_keys(arb)
        unused = sorted(k for k in keys if k not in used_keys)
        total_unused += len(unused)
        report[str(arb)] = {
            "unused": unused,
            "used_count": len(keys) - len(unused),
            "total": len(keys),
        }

    # Print human-readable report
    print("Localization Unused Keys Audit")
    print("-" * 40)
    for arb, info in report.items():
        print(f"File: {arb}")
        print(f"  Total keys: {info['total']}")
        print(f"  Used keys : {info['used_count']}")
        print(f"  Unused    : {len(info['unused'])}")
        if info["unused"]:
            for k in info["unused"]:
                print(f"    - {k}")
        print()

    if args.report_json:
        try:
            args.report_json.parent.mkdir(parents=True, exist_ok=True)
            with args.report_json.open("w", encoding="utf-8") as f:
                json.dump(report, f, ensure_ascii=False, indent=2)
            print(f"Report written to: {args.report_json}")
        except Exception as e:
            print(f"[WARN] Failed to write JSON report: {e}", file=sys.stderr)

    if total_unused > 0:
        print(f"Found {total_unused} unused keys across {len(arb_files)} ARB files.")
        return 1
    print("No unused keys found.")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        print("Interrupted.")
        sys.exit(130)
