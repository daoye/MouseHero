import 'dart:io';
import 'dart:convert';
import 'package:path/path.dart' as path;
import 'package:path_provider/path_provider.dart';
import 'package:flutter/services.dart' show rootBundle;

class ConfigService {
  static const String configFileName = 'server.conf';

  Future<String> readConfigFile() async {
    final configPath = await getConfigFile();
    final file = File(configPath);
    if (await file.exists()) {
      return await file.readAsString();
    }

    throw Exception('Config file not found at $configPath');
  }

  /// 解析配置文件内容为Map结构
  /// 返回格式为 {group: {key: value}}
  Future<Map<String, Map<String, String>>> parseConfig() async {
    final String content = await readConfigFile();
    return _parseConfigContent(content);
  }

  /// 解析配置文件内容
  Map<String, Map<String, String>> _parseConfigContent(String content) {
    final Map<String, Map<String, String>> result = {};
    String currentGroup = '';

    final lines = LineSplitter.split(content).toList();

    for (final line in lines) {
      final trimmedLine = line.trim();

      // 跳过空行和注释
      if (trimmedLine.isEmpty || trimmedLine.startsWith('#')) {
        continue;
      }

      // 解析分组
      if (trimmedLine.startsWith('[') && trimmedLine.endsWith(']')) {
        currentGroup = trimmedLine.substring(1, trimmedLine.length - 1);
        result[currentGroup] = {};
        continue;
      }

      // 解析键值对
      if (currentGroup.isNotEmpty && trimmedLine.contains('=')) {
        final parts = trimmedLine.split('=');
        if (parts.length >= 2) {
          final key = parts[0].trim();
          final value = parts.sublist(1).join('=').trim();
          result[currentGroup]![key] = value;
        }
      }
    }

    return result;
  }

  /// 按group和key获取配置值
  Future<String?> getValue(String group, String key) async {
    final config = await parseConfig();
    return config[group]?[key];
  }

  /// 获取指定group的所有配置
  Future<Map<String, String>?> getGroupValues(String group) async {
    final config = await parseConfig();
    return config[group];
  }

  Future<void> writeConfigFile(String content) async {
    try {
      final file = await _resolvePreferredConfigFile(createIfMissing: true);
      await file.writeAsString(content);
    } catch (e) {
      throw Exception('Failed to write to config file: $e');
    }
  }

  /// 返回最终配置文件：
  /// - Windows: 仅从可执行文件同级目录读取 server.conf（不自动创建/释放）
  /// - macOS:   ~/Library/MouseHero/server.conf（不存在则初始化）
  /// - Linux: 应用支持目录/MouseHero/server.conf（不存在则初始化）
  Future<File> _resolvePreferredConfigFile({
    bool createIfMissing = false,
  }) async {
    bool isDebugMode = false;
    assert(() {
      isDebugMode = true;
      return true;
    }());
    if (isDebugMode) {
      final projectRoot = _findProjectRoot();
      return File(path.join(projectRoot, 'server', 'server.conf'));
    }

    if (Platform.isMacOS) {
      final supportDir = await getLibraryDirectory();
      final appDir = Directory(path.join(supportDir.path, 'MouseHero'));
      if (!await appDir.exists()) {
        if (createIfMissing) {
          await appDir.create(recursive: true);
        }
      }
      final dest = File(path.join(appDir.path, configFileName));

      if (createIfMissing && !await dest.exists()) {
        // 优先从 .app/Contents/Resources/server.conf 初始化；失败则回退到 Flutter 资源
        String? template;
        try {
          template = await _readBundleDefaultTemplateMac();
        } catch (_) {
          // ignore and fallback
        }
        template ??= await _readAssetDefaultTemplate();
        await dest.writeAsString(template, flush: true);
      }

      return dest;
    } else if (Platform.isWindows) {
      return File(path.join(Directory.current.path, configFileName));
    }

    final appDir = await getApplicationSupportDirectory();
    if (!await appDir.exists()) {
      if (createIfMissing) {
        await appDir.create(recursive: true);
      }
    }
    final dest = File(path.join(appDir.path, configFileName));

    if (createIfMissing && !await dest.exists()) {
      final template = await _readAssetDefaultTemplate();
      await dest.writeAsString(template, flush: true);
    }

    return dest;
  }

  /// 读取 macOS 应用包内默认模板：.app/Contents/Resources/server.conf
  Future<String> _readBundleDefaultTemplateMac() async {
    // Platform.resolvedExecutable 指向 .app/Contents/MacOS/<exec>
    final execDir = path.dirname(Platform.resolvedExecutable);
    final resourcesDir = path.normalize(path.join(execDir, '..', 'Resources'));
    final templatePath = path.join(resourcesDir, configFileName);
    final file = File(templatePath);
    if (await file.exists()) {
      return await file.readAsString();
    }
    throw Exception('Default template not found at: $templatePath');
  }

  /// 读取 Flutter 资源中的默认模板：assets/default.conf
  Future<String> _readAssetDefaultTemplate() async {
    return await rootBundle.loadString('assets/default.conf');
  }

  String _findProjectRoot() {
    Directory dir = Directory(Directory.current.path);
    for (int i = 0; i < 15; i++) {
      final base = path.basename(dir.path);
      if (base == 'mousehero') {
        return dir.path;
      }
      final parent = dir.parent;
      if (parent.path == dir.path) {
        break;
      }
      dir = parent;
    }
    return Directory.current.path;
  }

  /// 获取配置文件路径（统一策略）：
  /// - 全平台：返回用户目录下的首选位置；若不存在则自动初始化默认模板
  Future<String> getConfigFile() async {
    final file = await _resolvePreferredConfigFile(createIfMissing: true);
    return file.path;
  }
}
