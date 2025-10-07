import 'dart:async';
import 'dart:convert';
import 'dart:io';
import 'package:flutter/foundation.dart';
import 'package:path/path.dart' as path;
import 'package:server_gui/services/config_service.dart';
import 'package:server_gui/services/settings_service.dart';
import 'package:server_gui/services/scripts_service.dart';
import 'package:path_provider/path_provider.dart';

class ServerProcessService {
  static final ServerProcessService _instance =
      ServerProcessService._internal();

  factory ServerProcessService() {
    return _instance;
  }

  ServerProcessService._internal();

  final StreamController<bool> _runningStateController =
      StreamController<bool>.broadcast();

  Stream<bool> get stateStream => _runningStateController.stream;

  Process? _serverProcess;
  StreamSubscription? _stdoutSubscription;
  StreamSubscription? _stderrSubscription;
  final List<String> _logBuffer = [];
  final int _maxLogBufferSize = 1000;
  final SettingsService _settingsService = SettingsService();
  // 守护与重启控制
  Timer? _restartTimer;
  bool _autoRestart = false; // 启动后守护进程，崩溃自动重启；手动停止会关闭该标志
  bool _stopping = false; // 手动停止标志
  int _restartAttempts = 0; // 指数退避计数

  bool get isRunning => _serverProcess != null;

  void _notifyRunningStateChanged(bool isRunning) {
    _runningStateController.add(isRunning);
  }

  Future<void> startServer() async {
    if (isRunning) {
      return;
    }
    _autoRestart = true;
    _stopping = false;
    _restartAttempts = 0;
    await _startOnce();
  }

  Future<void> _startOnce() async {
    try {
      await ScriptsService().ensureScriptsReady();
      final serverPath = await _getServerExecutablePath();
      final cfgPath = await ConfigService().getConfigFile();

      _serverProcess = await Process.start(
        serverPath,
        [cfgPath],
        mode: ProcessStartMode.normal,
        runInShell: false,
        workingDirectory: path.dirname(serverPath),
      );

      _stdoutSubscription = _serverProcess!.stdout
          .transform(utf8.decoder)
          .transform(const LineSplitter())
          .listen((line) {
            _addToLogBuffer(line);
          });

      _stderrSubscription = _serverProcess!.stderr
          .transform(utf8.decoder)
          .transform(const LineSplitter())
          .listen((line) {
            _addToLogBuffer(line);
          });

      _serverProcess!.exitCode.then((exitCode) async {
        _cleanupProcess();

        if (exitCode != 0) {
          _addToLogBuffer('Server exited with code: $exitCode');
        }

        await _cleanupPidFile();

        _notifyRunningStateChanged(false);

        if (_autoRestart && !_stopping) {
          _scheduleRestart();
        }
      });

      await _recordServerPid(_serverProcess!.pid);

      await _settingsService.setServerLastRunning(true);
      _notifyRunningStateChanged(true);
      _restartAttempts = 0;
    } catch (e) {
      _cleanupProcess();
      // 启动失败也尝试重启（如果开启守护且非手动停止）
      if (_autoRestart && !_stopping) {
        _addToLogBuffer('Failed to start server: $e');
        _scheduleRestart();
      }
      rethrow;
    }
  }

  Future<void> stopServer({bool manualStop = true}) async {
    if (!isRunning) {
      return;
    }

    try {
      _stopping = true;
      _autoRestart = false;
      _restartTimer?.cancel();
      _serverProcess!.kill();

      await _serverProcess!.exitCode.timeout(
        const Duration(seconds: 5),
        onTimeout: () {
          _serverProcess!.kill(ProcessSignal.sigkill);
          return -1;
        },
      );
    } catch (e) {
      rethrow;
    } finally {
      _cleanupProcess();
      await _cleanupPidFile();
      if (manualStop) {
        _settingsService.setServerLastRunning(false);
      }

      _notifyRunningStateChanged(false);
      _restartAttempts = 0;
    }
  }

  Future<void> restartServer() async {
    if (isRunning) {
      await stopServer();
    }
    await startServer();
  }

  void _cleanupProcess() {
    _stdoutSubscription?.cancel();
    _stdoutSubscription = null;

    _stderrSubscription?.cancel();
    _stderrSubscription = null;

    _serverProcess = null;
  }

  void _scheduleRestart() {
    _restartTimer?.cancel();
    // 简单指数退避：1s, 2s, 5s, 10s, 30s（封顶）
    final List<Duration> delays = [
      const Duration(seconds: 1),
      const Duration(seconds: 2),
      const Duration(seconds: 5),
      const Duration(seconds: 10),
      const Duration(seconds: 30),
    ];
    final delay = delays[(_restartAttempts).clamp(0, delays.length - 1)];
    _restartAttempts = (_restartAttempts + 1).clamp(0, 1000);
    _addToLogBuffer('Server will restart in ${delay.inSeconds}s');
    _restartTimer = Timer(delay, () async {
      if (_autoRestart && !_stopping && !isRunning) {
        await _startOnce();
      }
    });
  }

  List<String> getLogs() {
    return List.from(_logBuffer);
  }

  void clearLogs() {
    _logBuffer.clear();
  }

  void _addToLogBuffer(String line) {
    _logBuffer.add(line);

    if (_logBuffer.length > _maxLogBufferSize) {
      _logBuffer.removeAt(0);
    }
  }

  Future<String> _getServerExecutablePath() async {
    bool isDebugMode = false;
    assert(() {
      isDebugMode = true;
      return true;
    }());

    String serverExecutableName = Platform.isWindows ? 'server.exe' : 'server';
    String serverPath = '';

    if (isDebugMode) {
      String platform = '';
      String arch = '';

      if (Platform.isWindows) {
        platform = 'windows';
      } else if (Platform.isMacOS) {
        platform = 'macos';
      } else if (Platform.isLinux) {
        platform = 'linux';
      } else {
        throw UnsupportedError('不支持的平台: ${Platform.operatingSystem}');
      }

      arch = Platform.version.contains('arm') ? 'arm64' : 'x86_64';

      String projectRoot = _findProjectRoot();

      if (Platform.isMacOS) {
        serverPath = path.join(
          projectRoot,
          'server',
          'build',
          '$platform-$arch',
          'bin',
          'server.app',
          'Contents',
          'MacOS',
          serverExecutableName,
        );
      } else {
        serverPath = path.join(
          projectRoot,
          'server',
          'build',
          '$platform-$arch',
          'bin',
          serverExecutableName,
        );
      }
    } else {
      if (Platform.isMacOS) {
        // In release mode on macOS, the structure is:
        // MouseHero.app/Contents/MacOS/MouseHero (executable)
        // MouseHero.app/Contents/Resources/server.app/Contents/MacOS/server
        final appDir = path.dirname(path.dirname(Platform.resolvedExecutable));
        serverPath = path.join(
          appDir,
          'Resources',
          'server.app',
          'Contents',
          'MacOS',
          serverExecutableName,
        );
      } else {
        final execDir = path.dirname(Platform.resolvedExecutable);
        serverPath = path.join(execDir, serverExecutableName);
      }
    }

    return serverPath;
  }

  Future<void> cleanupOrphanedProcesses() async {
    try {
      final pidFilePath = await _getPidFilePath();
      final pidFile = File(pidFilePath);

      if (!await pidFile.exists()) {
        return;
      }
      final String pidStr = await pidFile.readAsString().catchError((e) {
        return '';
      });
      final int? lastPid = int.tryParse(pidStr.trim());
      if (lastPid == null) {
        await _cleanupPidFile();
        return;
      }

      final bool isRunning = await _isProcessRunning(lastPid);
      if (!isRunning) {
        await _cleanupPidFile();
        return;
      }

      final serverPath = await _getServerExecutablePath();
      final serverDir = path.dirname(serverPath);

      final bool isOurServer = await _isOurServerProcess(
        lastPid,
        serverPath,
        serverDir,
      );
      if (!isOurServer) {
        await _cleanupPidFile();
        return;
      }

      await _killProcess(lastPid);

      await _cleanupPidFile();
    } catch (e) {
      await _cleanupPidFile().catchError((_) {});
    }
  }

  Future<bool> _isProcessRunning(int pid) async {
    if (Platform.isWindows) {
      final result = await Process.run('tasklist', [
        '/FI',
        'PID eq $pid',
        '/NH',
      ]);
      return result.stdout.toString().contains('$pid');
    } else {
      return await Process.run('kill', [
        '-0',
        '$pid',
      ], runInShell: true).then((_) => true).catchError((_) => false);
    }
  }

  Future<bool> _isOurServerProcess(
    int pid,
    String serverPath,
    String serverDir,
  ) async {
    if (Platform.isWindows) {
      final result = await Process.run('wmic', [
        'process',
        'where',
        'ProcessId=$pid',
        'get',
        'CommandLine',
        '/format:list',
      ]);
      return result.stdout.toString().contains(serverPath);
    } else {
      final result = await Process.run('ps', ['-p', '$pid', '-o', 'command']);
      final cmdLine = result.stdout.toString();
      return cmdLine.contains(serverPath) || cmdLine.contains(serverDir);
    }
  }

  Future<bool> _killProcess(int pid) async {
    if (Platform.isWindows) {
      final result = await Process.run('taskkill', ['/F', '/PID', '$pid']);
      return result.exitCode == 0;
    } else {
      final result = await Process.run('kill', ['-9', '$pid']);
      return result.exitCode == 0;
    }
  }

  Future<void> _recordServerPid(int pid) async {
    try {
      final pidFilePath = await _getPidFilePath();
      final pidFile = File(pidFilePath);
      await pidFile.parent.create(recursive: true);
      await pidFile.writeAsString(pid.toString());
    } catch (e) {
      debugPrint('记录 PID 失败: $e');
    }
  }

  Future<String> _getPidFilePath() async {
    if (Platform.isWindows) {
      final appSupportDir = await getApplicationSupportDirectory();
      return path.join(appSupportDir.path, 'server.pid');
    } else if (Platform.isMacOS) {
      final libDir = await getLibraryDirectory();
      final appDir = Directory(path.join(libDir.path, 'MouseHero'));
      try {
        if (!await appDir.exists()) {
          await appDir.create(recursive: true);
        }
      } catch (_) {}
      return path.join(appDir.path, 'server.pid');
    } else {
      // Linux 使用 XDG runtime 目录
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

      return '$runtimeDir/server.pid';
    }
  }

  void dispose() {
    _autoRestart = false;
    _stopping = true;
    _runningStateController.close();
  }

  Future<void> _cleanupPidFile() async {
    try {
      final pidFilePath = await _getPidFilePath();
      final pidFile = File(pidFilePath);
      if (await pidFile.exists()) {
        await pidFile.delete();
      }
    } catch (e) {
      debugPrint('清理 PID 文件失败: $e');
    }
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
}
