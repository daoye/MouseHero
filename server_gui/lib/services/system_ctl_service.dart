import 'dart:io';

class SystemCtlService {
  Future<void> startServer() async {
    await _runCommand('start');
  }

  Future<void> stopServer() async {
    await _runCommand('stop');
  }

  Future<void> restartServer() async {
    await _runCommand('restart');
  }

  Future<void> _runCommand(String action) async {
    String command;
    List<String> arguments;

    if (Platform.isLinux) {
      command = 'systemctl';
      arguments = [action, 'mousehero.service'];
    } else if (Platform.isWindows) {
      command = 'sc';
      arguments = [action, 'mousehero'];
    } else if (Platform.isMacOS) {
      // This is a simplified example. Real implementation might be more complex.
      command = 'launchctl';
      if (action == 'start') {
        arguments = [
          'load',
          '/Library/LaunchDaemons/com.mousehero.server.plist',
        ];
      } else if (action == 'stop') {
        arguments = [
          'unload',
          '/Library/LaunchDaemons/com.mousehero.server.plist',
        ];
      } else {
        // restart
        await stopServer();
        await startServer();
        return;
      }
    } else {
      throw UnsupportedError(
        'Unsupported platform: ${Platform.operatingSystem}',
      );
    }

    final result = await Process.run(command, arguments);

    if (result.exitCode != 0) {
      throw Exception('Failed to $action server: ${result.stderr}');
    }
  }
}
