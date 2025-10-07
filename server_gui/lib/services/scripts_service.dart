import 'dart:io';
import 'package:flutter/foundation.dart';
import 'package:path/path.dart' as p;
import 'package:path_provider/path_provider.dart';

/// 负责在 macOS 上将应用包内的脚本复制到用户可修改的目录
/// - 源目录（Release）：.app/Contents/Resources/scripts
/// - 目标目录：~/Library/MouseHero/scripts
///
/// 复制策略：
/// - 若目标目录不存在，则从源目录整体复制（保留目录结构与可执行权限）
/// - 若目标已存在，则不覆盖用户修改，仅确保目录存在
class ScriptsService {
  static final ScriptsService _instance = ScriptsService._internal();
  factory ScriptsService() => _instance;
  ScriptsService._internal();

  /// 确保脚本已准备就绪（macOS 与 Linux）：
  /// - macOS: 将 .app/Contents/Resources/scripts 复制到 ~/Library/MouseHero/scripts
  /// - Linux: 将 可执行目录下的 scripts/ 复制到 AppSupport/MouseHero/scripts
  Future<void> ensureScriptsReady() async {
    try {
      if (Platform.isMacOS) {
        final destDir = await _getUserScriptsDirMac();
        if (await destDir.exists()) return;
        final srcDir = await _getBundleScriptsDirMac();
        if (srcDir == null || !await srcDir.exists()) return;
        await _copyDirectory(srcDir, destDir);
        await _fixExecutablePermissions(destDir);
        return;
      }

      if (Platform.isLinux) {
        final destDir = await _getUserScriptsDirLinux();
        if (await destDir.exists()) return;
        final srcDir = await _getExecSiblingScriptsDir();
        if (srcDir == null || !await srcDir.exists()) return;
        await _copyDirectory(srcDir, destDir);
        await _fixExecutablePermissions(destDir);
        return;
      }
    } catch (e) {
      debugPrint('ensureScriptsReady 失败: $e');
    }
  }

  /// macOS: ~/Library/MouseHero/scripts
  Future<Directory> _getUserScriptsDirMac() async {
    final libDir = await getLibraryDirectory();
    final appDir = Directory(p.join(libDir.path, 'MouseHero'));
    if (!await appDir.exists()) {
      await appDir.create(recursive: true);
    }
    final scriptsDir = Directory(p.join(appDir.path, 'scripts'));
    return scriptsDir;
  }

  /// .app/Contents/Resources/scripts（仅 macOS 打包形态可解析）
  Future<Directory?> _getBundleScriptsDirMac() async {
    try {
      // Platform.resolvedExecutable -> .app/Contents/MacOS/<exec>
      final execDir = p.dirname(Platform.resolvedExecutable);
      final resourcesDir = p.normalize(p.join(execDir, '..', 'Resources'));
      final scriptsPath = p.join(resourcesDir, 'scripts');
      final dir = Directory(scriptsPath);
      if (await dir.exists()) return dir;
      return null;
    } catch (_) {
      return null;
    }
  }

  /// Linux: AppSupport/MouseHero/scripts
  Future<Directory> _getUserScriptsDirLinux() async {
    final supportDir = await getApplicationSupportDirectory();
    final appDir = Directory(p.join(supportDir.path, 'MouseHero'));
    if (!await appDir.exists()) {
      await appDir.create(recursive: true);
    }
    final scriptsDir = Directory(p.join(appDir.path, 'scripts'));
    return scriptsDir;
  }

  /// Linux 源脚本目录：可执行文件同级的 scripts/
  Future<Directory?> _getExecSiblingScriptsDir() async {
    try {
      final execDir = p.dirname(Platform.resolvedExecutable);
      final scriptsPath = p.join(execDir, 'scripts');
      final dir = Directory(scriptsPath);
      if (await dir.exists()) return dir;
      return null;
    } catch (_) {
      return null;
    }
  }

  Future<void> _copyDirectory(Directory src, Directory dst) async {
    await dst.create(recursive: true);

    await for (final entity in src.list(recursive: true, followLinks: false)) {
      final relPath = p.relative(entity.path, from: src.path);
      final newPath = p.join(dst.path, relPath);

      if (entity is File) {
        await File(newPath).parent.create(recursive: true);
        await entity.copy(newPath);
      } else if (entity is Directory) {
        await Directory(newPath).create(recursive: true);
      }
    }
  }

  /// 设置 .sh/.command 无扩展可执行文件权限（755）
  Future<void> _fixExecutablePermissions(Directory dir) async {
    if (!Platform.isMacOS && !Platform.isLinux) return;

    await for (final entity in dir.list(recursive: true, followLinks: false)) {
      if (entity is File) {
        final name = p.basename(entity.path);
        final lower = name.toLowerCase();
        final isScript = lower.endsWith('.sh') || lower.endsWith('.command') || !lower.contains('.');
        if (isScript) {
          try {
            // 使用 chmod +x，避免 dart:io 无法直接设置 mode 的限制
            final result = await Process.run('chmod', ['755', entity.path]);
            if (result.exitCode != 0) {
              debugPrint('chmod 失败: ${entity.path} -> ${result.stderr}');
            }
          } catch (e) {
            debugPrint('设置可执行权限失败: ${entity.path}, $e');
          }
        }
      }
    }
  }
}
