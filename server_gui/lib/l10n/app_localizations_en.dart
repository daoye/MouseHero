// ignore: unused_import
import 'package:intl/intl.dart' as intl;
import 'app_localizations.dart';

// ignore_for_file: type=lint

/// The translations for English (`en`).
class AppLocalizationsEn extends AppLocalizations {
  AppLocalizationsEn([String locale = 'en']) : super(locale);

  @override
  String get appTitle => 'MouseHero';

  @override
  String get status => 'Status';

  @override
  String get running => 'RUNNING';

  @override
  String get stopped => 'STOPPED';

  @override
  String get address => 'Address';

  @override
  String get stopServer => 'Stop Server';

  @override
  String get startServer => 'Start Server';

  @override
  String get scanToConnect => 'Scan with MouseHero App to connect';

  @override
  String get logs => 'Logs';

  @override
  String clients(Object count) {
    return '$count clients';
  }

  @override
  String get settings => 'Settings';

  @override
  String get themeMode => 'Theme Mode';

  @override
  String get themeSystem => 'System Default';

  @override
  String get themeLight => 'Light Mode';

  @override
  String get themeDark => 'Dark Mode';

  @override
  String get language => 'Language';

  @override
  String get languageEnglish => 'English';

  @override
  String get languageChinese => 'Chinese';

  @override
  String get about => 'About MouseHero';

  @override
  String get loading => 'Loading...';

  @override
  String get dashboardServerStatus => 'Server Status';

  @override
  String get dashboardScanToConnect => 'Scan to Connect';

  @override
  String get dashboardServiceControl => 'Service Control';

  @override
  String dashboardOnlineClients(Object count) {
    return 'Online Clients ($count)';
  }

  @override
  String get dashboardStatusConnected => 'Connected';

  @override
  String get dashboardStatusNotConnected => 'Not Connected';

  @override
  String get dashboardStatusNoData => 'No Data';

  @override
  String get dashboardStatusRunning => 'Running';

  @override
  String get dashboardStatusStopped => 'Stopped';

  @override
  String get dashboardButtonStop => 'Stop';

  @override
  String get dashboardButtonRestart => 'Restart';

  @override
  String get dashboardButtonStart => 'Start Service';

  @override
  String get starting => 'Starting...';

  @override
  String get dashboardActionSuccessStop => 'Server stopped successfully.';

  @override
  String get dashboardActionSuccessRestart => 'Server restarted successfully.';

  @override
  String get dashboardActionSuccessStart => 'Server started successfully.';

  @override
  String dashboardActionFailed(Object error) {
    return 'Action failed: $error';
  }

  @override
  String get dashboardStatusError => 'Error loading dashboard';

  @override
  String get retry => 'Retry';

  @override
  String get dashboardNoClients => 'No online clients';

  @override
  String configLoadFailed(Object error) {
    return 'Failed to load config: $error';
  }

  @override
  String get configSaveSuccess => 'Config saved successfully!';

  @override
  String configSaveFailed(Object error) {
    return 'Failed to save config: $error';
  }

  @override
  String get configReset => 'Reset';

  @override
  String get configSave => 'Save Changes';

  @override
  String get configLabel => 'Server Configuration (server.conf)';

  @override
  String get logPageTitle => 'Logs';

  @override
  String get logLoadingError => 'Failed to load logs';

  @override
  String get logTitle => 'Logs';

  @override
  String get logNoRecords => 'No log records found.';

  @override
  String get reset => 'Reset';

  @override
  String get saveChanges => 'Save Changes';

  @override
  String get configResetSuccess => 'Configuration has been reset to default.';

  @override
  String configResetFailed(Object error) {
    return 'Failed to reset configuration: $error';
  }

  @override
  String get logClearTooltip => 'Clear Logs';

  @override
  String get logAutoRefresh => 'Auto-refresh';

  @override
  String get logClearSuccess => 'Logs have been cleared.';

  @override
  String logClearFailed(Object error) {
    return 'Failed to clear logs: $error';
  }

  @override
  String get notAvailable => 'N/A';

  @override
  String get sessionLabel => 'Session';

  @override
  String get commandLabel => 'Command';

  @override
  String get httpLabel => 'HTTP';

  @override
  String get ipLabel => 'IP';

  @override
  String get uidLabel => 'UID';

  @override
  String get durationLabel => 'Duration';

  @override
  String get hourUnit => 'h';

  @override
  String get minuteUnit => 'm';

  @override
  String get secondUnit => 's';

  @override
  String get logNoInfo => 'No log information available';

  @override
  String get logRefreshTooltip => 'Refresh logs';

  @override
  String get navDashboard => 'Dashboard';

  @override
  String get navConfig => 'Server Config';

  @override
  String get navLogs => 'Logs';

  @override
  String get navSettings => 'Settings';

  @override
  String get aboutPermissionsMenu => 'How to Set Permissions';

  @override
  String get settingsLicense => 'Open Source Licenses';

  @override
  String get settingsCheckForUpdates => 'Check for Updates';

  @override
  String settingsAuthor(String authorName) {
    return 'Author: $authorName';
  }

  @override
  String settingsCopyright(String authorName) {
    return 'Copyright © 2024 $authorName';
  }

  @override
  String get configResetConfirmTitle => 'Reset Configuration';

  @override
  String get configResetConfirmContent =>
      'Are you sure you want to reset the configuration to default? This action cannot be undone.';

  @override
  String get cancel => 'Cancel';

  @override
  String get confirm => 'Confirm';

  @override
  String get startupOnBoot => 'Start on system boot';

  @override
  String get startupOnBootSuccess => 'Auto-start setting updated successfully';

  @override
  String startupOnBootFailed(Object error) {
    return 'Failed to update auto-start setting: $error';
  }

  @override
  String get ok => 'OK';

  @override
  String get connectionError => 'Connection Error';

  @override
  String get info => 'Info';

  @override
  String get trayShowMainWindow => 'Show Main Window';

  @override
  String get trayExit => 'Exit';

  @override
  String dashboardUpdateAvailable(String version) {
    return 'A new version $version is available.';
  }

  @override
  String get dashboardUpdateButton => 'Download';

  @override
  String get dashboardUpdateError => 'Failed to check updates.';

  @override
  String get hideOnStartup => 'Hide main window on startup';

  @override
  String get hideOnAutoStart => 'Hide main window when auto-started on boot';

  @override
  String get settingStartMinimized => 'Start minimized to tray';

  @override
  String get settingStartMinimizedSuccess =>
      'Start minimized setting updated successfully';

  @override
  String settingStartMinimizedFailed(Object error) {
    return 'Failed to update start minimized setting: $error';
  }

  @override
  String get linuxPermissionsTitle => 'Permissions Setup Required';

  @override
  String get linuxPermissionsOnlyLinux =>
      'This page is only available on Linux';

  @override
  String get linuxPermissionsSetupTitle => 'Setup Instructions';

  @override
  String get linuxPermissionsStep1 => '1. Add your user to the input group:';

  @override
  String get linuxPermissionsStep2 =>
      '2. Copy the udev rule content below (you will paste it into the file created in Step 3):';

  @override
  String get linuxPermissionsStep3 =>
      '3. Create (or open) the udev rule file, then paste the Step 2 content into it:';

  @override
  String get linuxPermissionsStep4 => '4. Reload udev rules:';

  @override
  String get linuxPermissionsStep5 =>
      '5. Enable the uinput kernel module (load now and configure auto-load on boot):';

  @override
  String get linuxPermissionsStep6 =>
      '6. Log out and back in (or reboot), launch MouseHero.';

  @override
  String get linuxPermissionsHelp => 'Help';

  @override
  String get linuxPermissionsCopied => 'Command copied to clipboard';

  @override
  String get linuxPermissionsRuleCopied => 'Rule copied to clipboard';
}
