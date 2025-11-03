import 'dart:io';
import 'package:fluent_ui/fluent_ui.dart';
import 'package:server_gui/l10n/app_localizations.dart';
import 'package:server_gui/pages/about_page.dart';
import 'package:server_gui/pages/config_page.dart';
import 'package:server_gui/pages/dashboard_page.dart';
import 'package:server_gui/pages/linux_permissions_page.dart';
import 'package:server_gui/pages/log_page.dart';
import 'package:server_gui/services/linux_permissions_service.dart';
import 'package:server_gui/services/server_process_service.dart';
import 'package:server_gui/services/settings_service.dart';

class HomePage extends StatefulWidget {
  const HomePage({super.key});

  @override
  State<HomePage> createState() => _HomePageState();
}

class _HomePageState extends State<HomePage> {
  int _currentIndex = 0;
  bool _hasPermissions = true;
  bool _isCheckingPermissions = true;

  @override
  void initState() {
    super.initState();
    if (Platform.isLinux) {
      _checkPermissions();
    } else {
      _isCheckingPermissions = false;
      // On non-Linux platforms, auto-start immediately
      _autoStartServerIfNeeded();
    }
  }

  Future<void> _checkPermissions() async {
    final service = LinuxPermissionsService();
    bool hasPermissions;

    try {
      hasPermissions = await service.hasWriteAccessToDevice();
    } catch (_) {
      hasPermissions = false;
    }

    if (!mounted) return;

    setState(() {
      _hasPermissions = hasPermissions;
      _isCheckingPermissions = false;
    });

    if (hasPermissions) {
      _autoStartServerIfNeeded();
    }
  }

  Future<void> _autoStartServerIfNeeded() async {
    try {
      final settingsService = SettingsService();
      final lastRunningState = await settingsService.getServerLastRunning();
      
      if (lastRunningState) {
        final serverProcessService = ServerProcessService();
        await serverProcessService.startServer();
      }
    } catch (e) {
      // Silently fail, user can manually start if needed
    }
  }

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context)!;

    // Show loading while checking permissions on Linux
    if (_isCheckingPermissions) {
      return const Center(child: ProgressRing());
    }

    final items = <NavigationPaneItem>[
      PaneItem(
        icon: const Icon(FluentIcons.home),
        title: Text(l10n.navDashboard),
        body: Padding(
          padding: EdgeInsets.all(16),
          child: Platform.isLinux && !_hasPermissions
              ? const LinuxPermissionsPage()
              : const DashboardPage(),
        ),
      ),
      PaneItem(
        icon: const Icon(FluentIcons.code_edit),
        title: Text(l10n.navConfig),
        body: Padding(
          padding: EdgeInsets.all(16),
          child: const ConfigPage(),
        ),
      ),
      PaneItem(
        icon: const Icon(FluentIcons.text_document),
        title: Text(l10n.navLogs),
        body: Padding(padding: EdgeInsets.all(16), child: const LogPage()),
      ),
      PaneItem(
        icon: const Icon(FluentIcons.settings),
        title: Text(l10n.navSettings),
        body: Padding(
          padding: EdgeInsets.all(16),
          child: const AboutPage(),
        ),
      ),
    ];

    return NavigationView(
      pane: NavigationPane(
        selected: _currentIndex,
        onChanged: (index) => setState(() => _currentIndex = index),
        displayMode: PaneDisplayMode.auto,
        items: items,
      ),
    );
  }
}
