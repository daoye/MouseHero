import 'dart:io';

/// Service for providing Linux uinput permission guidance
class LinuxPermissionsService {
  // Use group-based approach for better security
  static const String udevRuleContent =
      '''KERNEL=="uinput", SUBSYSTEM=="misc", MODE="0660", GROUP="input"''';
  static const String udevRulePath =
      '/etc/udev/rules.d/99-mousehero-uinput.rules';
  static const String groupName = 'input';

  /// Whether current platform is Linux
  bool get isLinux => Platform.isLinux;

  /// Get the udev rule content for display/copy
  String getUdevRuleContent() => udevRuleContent;

  /// Get the udev rule file path
  String getUdevRulePath() => udevRulePath;

  /// Best-effort check for write access to /dev/uinput
  Future<bool> hasWriteAccessToDevice() async {
    if (!isLinux) return true;

    final device = File('/dev/uinput');
    if (!await device.exists()) {
      return false;
    }

    try {
      final handle = await device.open(mode: FileMode.write);
      await handle.close();
      return true;
    } catch (_) {
      return false;
    }
  }
}
