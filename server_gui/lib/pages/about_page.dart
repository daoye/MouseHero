import 'dart:io' show Platform;

import 'package:fluent_ui/fluent_ui.dart';
import 'package:flutter/material.dart' show ThemeMode, showLicensePage;
import 'package:provider/provider.dart';
import 'package:server_gui/l10n/app_localizations.dart';
import 'package:server_gui/pages/linux_permissions_page.dart';
import 'package:server_gui/providers/settings_provider.dart';
import 'package:server_gui/services/startup_service.dart';
import 'package:url_launcher/url_launcher.dart';
import 'package:package_info_plus/package_info_plus.dart';

class AboutPage extends StatefulWidget {
  const AboutPage({super.key});

  @override
  State<AboutPage> createState() => _AboutPageState();
}

class _AboutPageState extends State<AboutPage> {
  final Uri websiteUri = Uri.parse('https://mousehero.aprilzz.com');
  final StartupService _startupService = StartupService();
  bool _isStartupEnabled = false;
  bool _isLoading = true;
  PackageInfo _packageInfo = PackageInfo(
    appName: '',
    packageName: '',
    version: '',
    buildNumber: '',
  );

  @override
  void initState() {
    super.initState();
    _loadStartupStatus();
    _initPackageInfo();
  }

  Future<void> _initPackageInfo() async {
    final info = await PackageInfo.fromPlatform();
    setState(() {
      _packageInfo = info;
    });
  }

  Future<void> _loadStartupStatus() async {
    try {
      final isEnabled = await _startupService.isEnabled();
      setState(() {
        _isStartupEnabled = isEnabled;
        _isLoading = false;
      });
    } catch (e) {
      setState(() {
        _isLoading = false;
      });
      // 错误处理在UI中显示
    }
  }

  Future<void> _toggleStartupStatus(bool value) async {
    setState(() {
      _isLoading = true;
    });

    try {
      await _startupService.setEnabled(value);
      setState(() {
        _isStartupEnabled = value;
        _isLoading = false;
      });
    } catch (e) {
      setState(() {
        _isLoading = false;
      });
    }
  }

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context);

    if (l10n == null) {
      return const Center(child: Text('Localization not available.'));
    }

    return Column(
      mainAxisAlignment: MainAxisAlignment.start,
      children: [
        Expanded(child: _buildSettingsForm(context, l10n)),
        _buildAboutSection(context, l10n),
      ],
    );
  }

  Widget _buildSettingsForm(BuildContext context, AppLocalizations l10n) {
    final settingsProvider = Provider.of<SettingsProvider>(context);

    return ListView(
      shrinkWrap: true,
      children: [
        // 主题模式设置
        ListTile(
          leading: const Icon(FluentIcons.color),
          title: Text(l10n.themeMode),
          trailing: DropDownButton(
            title: Text(_getThemeModeName(settingsProvider.themeMode, l10n)),
            items: [
              MenuFlyoutItem(
                leading: const Icon(FluentIcons.system, size: 16),
                text: Text(l10n.themeSystem),
                onPressed:
                    () => settingsProvider.setThemeMode(ThemeMode.system),
              ),
              MenuFlyoutItem(
                leading: const Icon(FluentIcons.sunny, size: 16),
                text: Text(l10n.themeLight),
                onPressed: () => settingsProvider.setThemeMode(ThemeMode.light),
              ),
              MenuFlyoutItem(
                leading: const Icon(FluentIcons.clear_night, size: 16),
                text: Text(l10n.themeDark),
                onPressed: () => settingsProvider.setThemeMode(ThemeMode.dark),
              ),
            ],
          ),
        ),
        const SizedBox(height: 8),
        // 语言设置
        ListTile(
          leading: const Icon(FluentIcons.locale_language),
          title: Text(l10n.language),
          trailing: DropDownButton(
            title: Text(_getLanguageName(settingsProvider.locale, context)),
            items: [
              MenuFlyoutItem(
                text: const Text('English'),
                onPressed: () => settingsProvider.setLocale(const Locale('en')),
              ),
              MenuFlyoutItem(
                text: const Text('中文'),
                onPressed: () => settingsProvider.setLocale(const Locale('zh')),
              ),
            ],
          ),
        ),

        const SizedBox(height: 8),
        // 开机启动设置
        ListTile(
          leading: const Icon(FluentIcons.power_button),
          title: Text(l10n.startupOnBoot),
          trailing:
              _isLoading
                  ? const SizedBox(
                    width: 20,
                    height: 20,
                    child: ProgressRing(strokeWidth: 2),
                  )
                  : ToggleSwitch(
                    checked: _isStartupEnabled,
                    onChanged: _toggleStartupStatus,
                  ),
        ),
        const SizedBox(height: 8),

        // ListTile(
        //   leading: const Icon(FluentIcons.hide3),
        //   title: Text(l10n.hideOnAutoStart),
        //   trailing: ToggleSwitch(
        //     checked: settingsProvider.hideOnAutoStart,
        //     onChanged: settingsProvider.setHideOnAutoStart,
        //   ),
        // ),
        // const SizedBox(height: 8),
        ListTile(
          leading: const Icon(FluentIcons.download),
          title: Text(l10n.settingsCheckForUpdates),
          subtitle: Padding(
            padding: const EdgeInsets.only(top: 6.0),
            child: Text(
              'v${_packageInfo.version}+${_packageInfo.buildNumber}',
              style: TextStyle(
                color:
                    (FluentTheme.of(context).typography.caption?.color)
                        ?.withValues(alpha: 0.8) ??
                    Colors.grey,
              ),
            ),
          ),
          trailing: const Icon(FluentIcons.chevron_right),
          onPressed: () => _launchUrl(websiteUri),
        ),
        const SizedBox(height: 8),
        if (Platform.isLinux) ...[
          ListTile(
            leading: const Icon(FluentIcons.permissions),
            title: Text(l10n.aboutPermissionsMenu),
            trailing: const Icon(FluentIcons.chevron_right),
            onPressed: () {
              Navigator.of(context).push(
                FluentPageRoute(
                  builder: (_) => const LinuxPermissionsPage(),
                ),
              );
            },
          ),
          const SizedBox(height: 8),
        ],
        // 开源协议
        ListTile(
          leading: const Icon(FluentIcons.document),
          title: Text(l10n.settingsLicense),
          trailing: const Icon(FluentIcons.chevron_right),
          onPressed: () {
            showLicensePage(
              context: context,
              applicationName: l10n.appTitle,
              applicationVersion:
                  'v${_packageInfo.version}+${_packageInfo.buildNumber}',
              applicationLegalese: l10n.settingsCopyright("MouseHero"),
            );
          },
        ),
      ],
    );
  }

  // 获取主题模式名称
  String _getThemeModeName(ThemeMode mode, AppLocalizations l10n) {
    switch (mode) {
      case ThemeMode.system:
        return l10n.themeSystem;
      case ThemeMode.light:
        return l10n.themeLight;
      case ThemeMode.dark:
        return l10n.themeDark;
    }
  }

  // 获取语言名称（当为系统语言时，依据当前上下文解析后的实际语言）
  String _getLanguageName(Locale? locale, BuildContext context) {
    final effectiveLocaleCode =
        locale?.languageCode ?? AppLocalizations.of(context)!.localeName;
    final languageCode = effectiveLocaleCode.split('_').first;
    return languageCode == 'zh' ? '中文' : 'English';
  }

  Widget _buildAboutSection(BuildContext context, AppLocalizations l10n) {
    const String authorName = 'MouseHero';

    return Padding(
      padding: const EdgeInsets.only(top: 16.0),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.center,
        spacing: 16,
        children: [
          Text(
            l10n.settingsAuthor(authorName),
            style: FluentTheme.of(context).typography.caption,
          ),
          Text(
            l10n.settingsCopyright(authorName),
            style: FluentTheme.of(context).typography.caption,
          ),
          GestureDetector(
            onTap: () => _launchUrl(websiteUri),
            child: Text(
              '$websiteUri',
              style: FluentTheme.of(context).typography.caption,
            ),
          ),
        ],
      ),
    );
  }

  Future<void> _launchUrl(Uri url) async {
    if (await canLaunchUrl(url)) {
      await launchUrl(url, mode: LaunchMode.externalApplication);
    } else {
      if (!mounted) return;
      final l10n = AppLocalizations.of(context)!;
      showDialog(
        context: context,
        builder:
            (context) => ContentDialog(
              title: Text(l10n.logLoadingError),
              content: Text(l10n.dashboardActionFailed(url.toString())),
              actions: [
                Button(
                  child: Text(l10n.retry),
                  onPressed: () => Navigator.of(context).pop(),
                ),
              ],
            ),
      );
    }
  }
}
