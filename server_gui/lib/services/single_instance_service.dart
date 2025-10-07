import 'dart:async';
import 'dart:io';
import 'package:flutter/foundation.dart';

import 'package:window_manager/window_manager.dart';
import 'package:flutter_alone/flutter_alone.dart';

class SingleInstanceService {
  static final SingleInstanceService _instance =
      SingleInstanceService._internal();

  factory SingleInstanceService() => _instance;

  SingleInstanceService._internal();

  Socket? _socket;
  ServerSocket? _serverSocket;
  StreamSubscription? _commandFileWatcher;

  FlutterAloneConfig? _config;

  static const String _lockfile = 'com.aprilzz.mousehero_gui.sock';

  String _socketPath = '';

  Future<bool> checkAlone() async {
    if (Platform.isWindows || Platform.isMacOS) {
      return await _checkoutWindwosAndMac();
    } else {
      return await _checkLinux();
    }
  }

  Future<bool> _checkoutWindwosAndMac() async {
    if (Platform.isWindows) {
      _config = FlutterAloneConfig.forWindows(
        windowsConfig: CustomWindowsMutexConfig(
          customMutexName: _lockfile,
        ),
        messageConfig: const EnMessageConfig(),
      );
    } else if (Platform.isMacOS) {
      _config = FlutterAloneConfig.forMacOS(
        macOSConfig: MacOSConfig(lockFileName: _lockfile),
        messageConfig: const EnMessageConfig(),
      );
    }

    var isfirst = await FlutterAlone.instance.checkAndRun(config: _config!);

    if (!isfirst) {
      _showAndFocusWindow();
    }

    return isfirst;
  }

  Future<bool> _checkLinux() async {
    await _initLinuxPaths();

    final socketFile = File(_socketPath);
    if (await socketFile.exists()) {
      try {
        _socket = await Socket.connect(
          InternetAddress(_socketPath, type: InternetAddressType.unix),
          0,
        );

        _socket!.add([1]);
        await _socket!.close();
        return false;
      } catch (connectError) {
        try {
          await socketFile.delete(recursive: true);
        } catch (deleteError) {
          return true;
        }
      }
    }

    try {
      _serverSocket = await ServerSocket.bind(
        InternetAddress(_socketPath, type: InternetAddressType.unix),
        0,
      );

      _serverSocket!.listen((client) {
        _showAndFocusWindow();
        client.close();
      });

      return true;
    } catch (e) {
      return true;
    }
  }

  Future<void> _initLinuxPaths() async {
    String runtimeDir = '/run/user/1000';

    final xdgRuntimeDir = Platform.environment['XDG_RUNTIME_DIR'];
    if (xdgRuntimeDir != null && xdgRuntimeDir.isNotEmpty) {
      runtimeDir = xdgRuntimeDir;
    } else {
      try {
        final result = await Process.run('id', ['-u']);
        if (result.exitCode == 0) {
          runtimeDir = '/run/user/${result.stdout.toString().trim()}';
        }
      } catch (e) {
        debugPrint('获取当前用户ID失败: $e');
      }
    }

    _socketPath = '$runtimeDir/$_lockfile';
  }

  Future<void> _showAndFocusWindow() async {
    await windowManager.show();
    await windowManager.focus();
  }

  Future<void> dispose() async {
    if (Platform.isLinux) {
      await _serverSocket?.close();
      await _socket?.close();
      await _commandFileWatcher?.cancel();

      final socketFile = File(_socketPath);
      if (await socketFile.exists()) {
        await socketFile.delete();
      }
    }

    if ((Platform.isWindows || Platform.isMacOS)) {
      await FlutterAlone.instance.dispose();
    }
  }
}

final singleInstanceService = SingleInstanceService();