import 'dart:io';
import 'package:launch_at_startup/launch_at_startup.dart';
import 'package:package_info_plus/package_info_plus.dart';

/// 处理应用程序开机启动的服务
class StartupService {
  static final StartupService _instance = StartupService._internal();
  factory StartupService() => _instance;
  StartupService._internal();

  bool _initialized = false;
  bool _isEnabled = false;

  /// 获取当前开机启动状态
  Future<bool> isEnabled() async {
    await _ensureInitialized();
    return _isEnabled;
  }

  /// 设置开机启动状态
  Future<void> setEnabled(bool enabled) async {
    await _ensureInitialized();

    try {
      if (enabled) {
        await launchAtStartup.enable();
      } else {
        await launchAtStartup.disable();
      }
      _isEnabled = enabled;
    } catch (e) {
      throw Exception(
        'Failed to ${enabled ? 'enable' : 'disable'} startup: $e',
      );
    }
  }

  /// 确保服务已初始化
  Future<void> _ensureInitialized() async {
    if (_initialized) return;

    final packageInfo = await PackageInfo.fromPlatform();

    launchAtStartup.setup(
      appName: packageInfo.appName,
      appPath: await _getAppPath(),
      args: ['--autostart'],
    );

    _isEnabled = await launchAtStartup.isEnabled();
    _initialized = true;
  }

  /// 获取应用程序路径
  Future<String> _getAppPath() async {
    if (Platform.isWindows) {
      // Windows 平台使用当前可执行文件路径
      return Platform.resolvedExecutable;
    } else if (Platform.isMacOS) {
      // macOS 平台需要返回 .app 包路径，确保登录项能携带参数并正确启动
      // Platform.resolvedExecutable 一般为: /Applications/MouseHero.app/Contents/MacOS/MouseHero
      // 因此向上三级目录可得到 .app 根路径
      final execFile = File(Platform.resolvedExecutable);
      final macOSDir = execFile.parent; // Contents/MacOS
      final contentsDir = macOSDir.parent; // Contents
      final appBundleDir = contentsDir.parent; // *.app
      return appBundleDir.path;
    } else if (Platform.isLinux) {
      // Linux 平台使用当前可执行文件路径
      return Platform.resolvedExecutable;
    } else {
      throw UnsupportedError(
        'Platform ${Platform.operatingSystem} is not supported for auto-start',
      );
    }
  }
}
