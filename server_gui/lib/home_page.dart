import 'package:fluent_ui/fluent_ui.dart';
import 'package:server_gui/l10n/app_localizations.dart';
import 'package:server_gui/pages/config_page.dart';
import 'package:server_gui/pages/dashboard_page.dart';
import 'package:server_gui/pages/log_page.dart';
import 'package:server_gui/pages/about_page.dart';

class HomePage extends StatefulWidget {
  const HomePage({super.key});

  @override
  State<HomePage> createState() => _HomePageState();
}

class _HomePageState extends State<HomePage> {
  int _currentIndex = 0;

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context)!;

    return NavigationView(
      pane: NavigationPane(
        selected: _currentIndex,
        onChanged: (index) => setState(() => _currentIndex = index),
        displayMode: PaneDisplayMode.auto,
        items: [
          PaneItem(
            icon: const Icon(FluentIcons.home),
            title: Text(l10n.navDashboard),
            body: Padding(
              padding: EdgeInsets.all(16),
              child: const DashboardPage(),
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
        ],
      ),
    );
  }
}
