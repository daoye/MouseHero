import 'dart:async';
import 'dart:io';
import 'package:flutter/widgets.dart';
import 'package:server_gui/l10n/app_localizations.dart';
import 'package:server_gui/services/server_process_service.dart';
import 'package:tray_manager/tray_manager.dart';
import 'package:window_manager/window_manager.dart';

class TrayService with TrayListener {
  static final TrayService _instance = TrayService._internal();
  factory TrayService() => _instance;

  /// Set to true when the user chooses Exit from the tray menu.
  /// This allows onWindowClose to perform a real shutdown instead of hiding.
  static bool exitRequested = false;

  bool _initialized = false;
  bool _isServerRunning = false;
  StreamSubscription<bool>? _serverStateSubscription;
  final ServerProcessService _serverProcessService = ServerProcessService();
  BuildContext? _lastContext;

  TrayService._internal() {
    trayManager.addListener(this);
    _initServerStateListener();
  }

  Future<void> createTray(String title) async {
    try {
      final String iconPath =
          Platform.isWindows
              ? 'assets/app_icon.ico'
              : 'assets/app_icon.png'; // await _getPlatformSpecificIconPath();

      await trayManager.setIcon(iconPath);

      if (!Platform.isLinux) {
        await trayManager.setToolTip(title);
      }

      _initialized = true;
    } catch (e) {
      _initialized = false;
    }
  }

  // Future<String> _getPlatformSpecificIconPath() async {
  //   const String pngAsset = 'assets/app_icon.png';
  //   const String icoAsset = 'assets/app_icon.ico';

  //   // On Windows/Linux, tray_manager expects a filesystem path.
  //   // We copy the bundled asset to the temp directory and use that path.
  //   if (Platform.isWindows || Platform.isLinux) {
  //     final bool isWindows = Platform.isWindows;
  //     final String chosenAsset = isWindows ? icoAsset : pngAsset;
  //     final String fileName =
  //         isWindows ? 'mousehero_tray_icon.ico' : 'mousehero_tray_icon.png';

  //     try {
  //       final Directory tempDir = await getTemporaryDirectory();
  //       final String tempPath = path.join(tempDir.path, fileName);
  //       final File tempFile = File(tempPath);

  //       if (!await tempFile.exists()) {
  //         final ByteData data = await rootBundle.load(chosenAsset);
  //         final List<int> bytes = data.buffer.asUint8List();
  //         await tempFile.writeAsBytes(bytes);
  //       }

  //       return tempPath;
  //     } catch (e) {
  //       // Fallback to asset path; on Windows/Linux this may not work,
  //       // but it's better than throwing.
  //       return chosenAsset;
  //     }
  //   }

  //   // On macOS and others, returning the asset path is acceptable.
  //   return pngAsset;
  // }

  void _initServerStateListener() {
    _serverStateSubscription = _serverProcessService.stateStream.listen((
      isRunning,
    ) {
      _isServerRunning = isRunning;
      if (_lastContext != null) {
        updateMenuAndTitle(_lastContext!);
      }
    });

    _isServerRunning = _serverProcessService.isRunning;
  }

  Future<void> _showWindow() async {
    await windowManager.setSkipTaskbar(false);
    await windowManager.show();
    await windowManager.focus();
  }

  Future<void> updateMenuAndTitle(BuildContext context) async {
    if (!_initialized) {
      return;
    }

    _lastContext = context;
    final l10n = AppLocalizations.of(context)!;

    Menu menu = Menu(
      items: [
        MenuItem(
          key: 'toggle_server',
          label: _isServerRunning ? l10n.stopServer : l10n.startServer,
          onClick: (_) async {
            try {
              if (_isServerRunning) {
                await _serverProcessService.stopServer();
              } else {
                await _serverProcessService.startServer();
              }
            } catch (e) {
              debugPrint('托盘服务控制错误: $e');
            }
          },
        ),
        MenuItem(
          key: 'show_window',
          label: l10n.trayShowMainWindow,
          onClick: (_) async {
            await _showWindow();
          },
        ),
        MenuItem(
          key: 'exit_app',
          label: l10n.trayExit,
          onClick: (_) async {
            TrayService.exitRequested = true;
            windowManager.close();
          },
        ),
      ],
    );

    await trayManager.setContextMenu(menu);
  }

  @override
  void onTrayIconMouseDown() {
    _showWindow();
  }

  @override
  void onTrayIconRightMouseDown() {
    trayManager.popUpContextMenu();
  }

  @override
  void onTrayMenuItemClick(MenuItem menuItem) {}

  void dispose() {
    trayManager.removeListener(this);
    _serverStateSubscription?.cancel();
    _lastContext = null;
  }
}
