import 'dart:io';
import 'package:fluent_ui/fluent_ui.dart';
import 'package:flutter_localizations/flutter_localizations.dart';
import 'package:provider/provider.dart';
import 'package:server_gui/home_page.dart';
import 'package:server_gui/l10n/app_localizations.dart';
import 'package:server_gui/providers/settings_provider.dart';
import 'package:server_gui/services/server_process_service.dart';
import 'package:server_gui/services/settings_service.dart';
import 'package:server_gui/services/single_instance_service.dart';
import 'package:server_gui/services/tray_service.dart';
import 'package:window_manager/window_manager.dart';

void main(List<String> args) async {
  WidgetsFlutterBinding.ensureInitialized();

  Directory.current = File(Platform.resolvedExecutable).parent;

  if (!await singleInstanceService.checkAlone()) {
    exit(0);
  }

  await windowManager.ensureInitialized();
  await windowManager.setMinimumSize(const Size(800, 800));
  await windowManager.setPreventClose(true);

  WindowOptions windowOptions = const WindowOptions(
    size: Size(800, 600),
    center: true,
    skipTaskbar: false,
    titleBarStyle: TitleBarStyle.normal,
  );

  final trayService = TrayService();
  await trayService.createTray("Mouse Hero");

  final settingsService = SettingsService();
  await settingsService.init();

  final hideOnAutoStart = true; // await settingsService.getHideOnAutoStart();

  final serverProcessService = ServerProcessService();

  bool isAutoStart = false;
  try {
    isAutoStart =
        args.any((x) => x.contains("autostart")) ||
        Platform.executableArguments.contains('autostart');
  } catch (e) {
    debugPrint('读取启动参数失败: $e');
  }

  // 在 macOS 上，系统登录项不会传递自定义参数。
  // 因此只要用户勾选了“开机自启时隐藏主界面”，我们就无条件隐藏启动。
  // 其他平台仍按 --autostart 参数判定是否隐藏。
  bool shouldHide = false;
  if (hideOnAutoStart) {
    if (Platform.isMacOS) {
      shouldHide = true;
    } else {
      shouldHide = isAutoStart;
    }
  }

  try {
    await serverProcessService.cleanupOrphanedProcesses();

    final lastRunningState = await settingsService.getServerLastRunning();
    if (lastRunningState) {
      await serverProcessService.startServer();
    }
  } catch (e) {
    debugPrint('启动或清理子进程失败: $e');
  }

  // if (shouldHide) {
  //   // 自启动且需要隐藏时，避免出现在任务栏/坞站
  //   await windowManager.setSkipTaskbar(true);
  // }

  windowManager.waitUntilReadyToShow(windowOptions, () async {
    if (!shouldHide) {
      await windowManager.show();
      await windowManager.focus();
    }
  });

  runApp(
    MultiProvider(
      providers: [ChangeNotifierProvider(create: (_) => SettingsProvider())],
      child: const MyApp(),
    ),
  );
}

class MyApp extends StatefulWidget {
  const MyApp({super.key});

  @override
  State<MyApp> createState() => _MyAppState();
}

class _MyAppState extends State<MyApp> with WindowListener {
  @override
  void initState() {
    super.initState();
    windowManager.addListener(this);
  }

  @override
  void dispose() {
    windowManager.removeListener(this);
    super.dispose();
  }

  @override
  void onWindowClose() async {
    if (!TrayService.exitRequested) {
      await windowManager.setSkipTaskbar(true);
      await windowManager.hide();
      return;
    }

    SingleInstanceService().dispose();
    TrayService().dispose();
    if(!Platform.isMacOS) {
      // Form macos termite child process in native swift code, But other false
      await ServerProcessService().stopServer(manualStop: false);
    }

    await windowManager.destroy();
  }

  @override
  Widget build(BuildContext context) {
    final settingsProvider = Provider.of<SettingsProvider>(context);

    return FluentApp(
      onGenerateTitle: (context) {
        final l10n = AppLocalizations.of(context)!;
        windowManager.setTitle(l10n.appTitle);

        WidgetsBinding.instance.addPostFrameCallback((_) async {
          if (mounted) {
            final trayService = TrayService();
            await trayService.updateMenuAndTitle(context);
          }
        });

        return l10n.appTitle;
      },
      locale: settingsProvider.locale,
      localizationsDelegates: const [
        AppLocalizations.delegate,
        GlobalMaterialLocalizations.delegate,
        GlobalWidgetsLocalizations.delegate,
        GlobalCupertinoLocalizations.delegate,
        FluentLocalizations.delegate,
      ],
      supportedLocales: AppLocalizations.supportedLocales,
      localeResolutionCallback: (deviceLocale, supportedLocales) {
        // 仅依据系统语言做解析，避免受 Provider 状态变化影响
        if (deviceLocale != null) {
          final lang = deviceLocale.languageCode.toLowerCase();
          if (lang == 'zh') {
            return const Locale('zh');
          }
        }
        return const Locale('en');
      },
      localeListResolutionCallback: (deviceLocales, supportedLocales) {
        // 仅依据系统语言列表做解析，避免受 Provider 状态变化影响
        if (deviceLocales != null && deviceLocales.isNotEmpty) {
          for (final l in deviceLocales) {
            if (l.languageCode.toLowerCase() == 'zh') {
              return const Locale('zh');
            }
          }
        }
        return const Locale('en');
      },
      theme: FluentThemeData(
        brightness: Brightness.light,
        accentColor: Colors.blue,
      ),
      darkTheme: FluentThemeData(
        brightness: Brightness.dark,
        accentColor: Colors.blue,
      ),
      themeMode: settingsProvider.themeMode,
      home: const HomePage(),
    );
  }
}
