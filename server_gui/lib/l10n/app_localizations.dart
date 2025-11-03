import 'dart:async';

import 'package:flutter/foundation.dart';
import 'package:flutter/widgets.dart';
import 'package:flutter_localizations/flutter_localizations.dart';
import 'package:intl/intl.dart' as intl;

import 'app_localizations_en.dart';
import 'app_localizations_zh.dart';

// ignore_for_file: type=lint

/// Callers can lookup localized strings with an instance of AppLocalizations
/// returned by `AppLocalizations.of(context)`.
///
/// Applications need to include `AppLocalizations.delegate()` in their app's
/// `localizationDelegates` list, and the locales they support in the app's
/// `supportedLocales` list. For example:
///
/// ```dart
/// import 'l10n/app_localizations.dart';
///
/// return MaterialApp(
///   localizationsDelegates: AppLocalizations.localizationsDelegates,
///   supportedLocales: AppLocalizations.supportedLocales,
///   home: MyApplicationHome(),
/// );
/// ```
///
/// ## Update pubspec.yaml
///
/// Please make sure to update your pubspec.yaml to include the following
/// packages:
///
/// ```yaml
/// dependencies:
///   # Internationalization support.
///   flutter_localizations:
///     sdk: flutter
///   intl: any # Use the pinned version from flutter_localizations
///
///   # Rest of dependencies
/// ```
///
/// ## iOS Applications
///
/// iOS applications define key application metadata, including supported
/// locales, in an Info.plist file that is built into the application bundle.
/// To configure the locales supported by your app, you’ll need to edit this
/// file.
///
/// First, open your project’s ios/Runner.xcworkspace Xcode workspace file.
/// Then, in the Project Navigator, open the Info.plist file under the Runner
/// project’s Runner folder.
///
/// Next, select the Information Property List item, select Add Item from the
/// Editor menu, then select Localizations from the pop-up menu.
///
/// Select and expand the newly-created Localizations item then, for each
/// locale your application supports, add a new item and select the locale
/// you wish to add from the pop-up menu in the Value field. This list should
/// be consistent with the languages listed in the AppLocalizations.supportedLocales
/// property.
abstract class AppLocalizations {
  AppLocalizations(String locale)
    : localeName = intl.Intl.canonicalizedLocale(locale.toString());

  final String localeName;

  static AppLocalizations? of(BuildContext context) {
    return Localizations.of<AppLocalizations>(context, AppLocalizations);
  }

  static const LocalizationsDelegate<AppLocalizations> delegate =
      _AppLocalizationsDelegate();

  /// A list of this localizations delegate along with the default localizations
  /// delegates.
  ///
  /// Returns a list of localizations delegates containing this delegate along with
  /// GlobalMaterialLocalizations.delegate, GlobalCupertinoLocalizations.delegate,
  /// and GlobalWidgetsLocalizations.delegate.
  ///
  /// Additional delegates can be added by appending to this list in
  /// MaterialApp. This list does not have to be used at all if a custom list
  /// of delegates is preferred or required.
  static const List<LocalizationsDelegate<dynamic>> localizationsDelegates =
      <LocalizationsDelegate<dynamic>>[
        delegate,
        GlobalMaterialLocalizations.delegate,
        GlobalCupertinoLocalizations.delegate,
        GlobalWidgetsLocalizations.delegate,
      ];

  /// A list of this localizations delegate's supported locales.
  static const List<Locale> supportedLocales = <Locale>[
    Locale('en'),
    Locale('zh'),
  ];

  /// No description provided for @appTitle.
  ///
  /// In en, this message translates to:
  /// **'MouseHero'**
  String get appTitle;

  /// No description provided for @status.
  ///
  /// In en, this message translates to:
  /// **'Status'**
  String get status;

  /// No description provided for @running.
  ///
  /// In en, this message translates to:
  /// **'RUNNING'**
  String get running;

  /// No description provided for @stopped.
  ///
  /// In en, this message translates to:
  /// **'STOPPED'**
  String get stopped;

  /// No description provided for @address.
  ///
  /// In en, this message translates to:
  /// **'Address'**
  String get address;

  /// No description provided for @stopServer.
  ///
  /// In en, this message translates to:
  /// **'Stop Server'**
  String get stopServer;

  /// No description provided for @startServer.
  ///
  /// In en, this message translates to:
  /// **'Start Server'**
  String get startServer;

  /// No description provided for @scanToConnect.
  ///
  /// In en, this message translates to:
  /// **'Scan with MouseHero App to connect'**
  String get scanToConnect;

  /// No description provided for @logs.
  ///
  /// In en, this message translates to:
  /// **'Logs'**
  String get logs;

  /// No description provided for @clients.
  ///
  /// In en, this message translates to:
  /// **'{count} clients'**
  String clients(Object count);

  /// No description provided for @settings.
  ///
  /// In en, this message translates to:
  /// **'Settings'**
  String get settings;

  /// No description provided for @themeMode.
  ///
  /// In en, this message translates to:
  /// **'Theme Mode'**
  String get themeMode;

  /// No description provided for @themeSystem.
  ///
  /// In en, this message translates to:
  /// **'System Default'**
  String get themeSystem;

  /// No description provided for @themeLight.
  ///
  /// In en, this message translates to:
  /// **'Light Mode'**
  String get themeLight;

  /// No description provided for @themeDark.
  ///
  /// In en, this message translates to:
  /// **'Dark Mode'**
  String get themeDark;

  /// No description provided for @language.
  ///
  /// In en, this message translates to:
  /// **'Language'**
  String get language;

  /// No description provided for @languageEnglish.
  ///
  /// In en, this message translates to:
  /// **'English'**
  String get languageEnglish;

  /// No description provided for @languageChinese.
  ///
  /// In en, this message translates to:
  /// **'Chinese'**
  String get languageChinese;

  /// No description provided for @about.
  ///
  /// In en, this message translates to:
  /// **'About MouseHero'**
  String get about;

  /// No description provided for @loading.
  ///
  /// In en, this message translates to:
  /// **'Loading...'**
  String get loading;

  /// No description provided for @dashboardServerStatus.
  ///
  /// In en, this message translates to:
  /// **'Server Status'**
  String get dashboardServerStatus;

  /// No description provided for @dashboardScanToConnect.
  ///
  /// In en, this message translates to:
  /// **'Scan to Connect'**
  String get dashboardScanToConnect;

  /// No description provided for @dashboardServiceControl.
  ///
  /// In en, this message translates to:
  /// **'Service Control'**
  String get dashboardServiceControl;

  /// No description provided for @dashboardOnlineClients.
  ///
  /// In en, this message translates to:
  /// **'Online Clients ({count})'**
  String dashboardOnlineClients(Object count);

  /// No description provided for @dashboardStatusConnected.
  ///
  /// In en, this message translates to:
  /// **'Connected'**
  String get dashboardStatusConnected;

  /// No description provided for @dashboardStatusNotConnected.
  ///
  /// In en, this message translates to:
  /// **'Not Connected'**
  String get dashboardStatusNotConnected;

  /// No description provided for @dashboardStatusNoData.
  ///
  /// In en, this message translates to:
  /// **'No Data'**
  String get dashboardStatusNoData;

  /// No description provided for @dashboardStatusRunning.
  ///
  /// In en, this message translates to:
  /// **'Running'**
  String get dashboardStatusRunning;

  /// No description provided for @dashboardStatusStopped.
  ///
  /// In en, this message translates to:
  /// **'Stopped'**
  String get dashboardStatusStopped;

  /// No description provided for @dashboardButtonStop.
  ///
  /// In en, this message translates to:
  /// **'Stop'**
  String get dashboardButtonStop;

  /// No description provided for @dashboardButtonRestart.
  ///
  /// In en, this message translates to:
  /// **'Restart'**
  String get dashboardButtonRestart;

  /// No description provided for @dashboardButtonStart.
  ///
  /// In en, this message translates to:
  /// **'Start Service'**
  String get dashboardButtonStart;

  /// No description provided for @starting.
  ///
  /// In en, this message translates to:
  /// **'Starting...'**
  String get starting;

  /// No description provided for @dashboardActionSuccessStop.
  ///
  /// In en, this message translates to:
  /// **'Server stopped successfully.'**
  String get dashboardActionSuccessStop;

  /// No description provided for @dashboardActionSuccessRestart.
  ///
  /// In en, this message translates to:
  /// **'Server restarted successfully.'**
  String get dashboardActionSuccessRestart;

  /// No description provided for @dashboardActionSuccessStart.
  ///
  /// In en, this message translates to:
  /// **'Server started successfully.'**
  String get dashboardActionSuccessStart;

  /// No description provided for @dashboardActionFailed.
  ///
  /// In en, this message translates to:
  /// **'Action failed: {error}'**
  String dashboardActionFailed(Object error);

  /// No description provided for @dashboardStatusError.
  ///
  /// In en, this message translates to:
  /// **'Error loading dashboard'**
  String get dashboardStatusError;

  /// No description provided for @retry.
  ///
  /// In en, this message translates to:
  /// **'Retry'**
  String get retry;

  /// No description provided for @dashboardNoClients.
  ///
  /// In en, this message translates to:
  /// **'No online clients'**
  String get dashboardNoClients;

  /// No description provided for @configLoadFailed.
  ///
  /// In en, this message translates to:
  /// **'Failed to load config: {error}'**
  String configLoadFailed(Object error);

  /// No description provided for @configSaveSuccess.
  ///
  /// In en, this message translates to:
  /// **'Config saved successfully!'**
  String get configSaveSuccess;

  /// No description provided for @configSaveFailed.
  ///
  /// In en, this message translates to:
  /// **'Failed to save config: {error}'**
  String configSaveFailed(Object error);

  /// No description provided for @configReset.
  ///
  /// In en, this message translates to:
  /// **'Reset'**
  String get configReset;

  /// No description provided for @configSave.
  ///
  /// In en, this message translates to:
  /// **'Save Changes'**
  String get configSave;

  /// No description provided for @configLabel.
  ///
  /// In en, this message translates to:
  /// **'Server Configuration (server.conf)'**
  String get configLabel;

  /// No description provided for @logPageTitle.
  ///
  /// In en, this message translates to:
  /// **'Logs'**
  String get logPageTitle;

  /// No description provided for @logLoadingError.
  ///
  /// In en, this message translates to:
  /// **'Failed to load logs'**
  String get logLoadingError;

  /// No description provided for @logTitle.
  ///
  /// In en, this message translates to:
  /// **'Logs'**
  String get logTitle;

  /// No description provided for @logNoRecords.
  ///
  /// In en, this message translates to:
  /// **'No log records found.'**
  String get logNoRecords;

  /// No description provided for @reset.
  ///
  /// In en, this message translates to:
  /// **'Reset'**
  String get reset;

  /// No description provided for @saveChanges.
  ///
  /// In en, this message translates to:
  /// **'Save Changes'**
  String get saveChanges;

  /// No description provided for @configResetSuccess.
  ///
  /// In en, this message translates to:
  /// **'Configuration has been reset to default.'**
  String get configResetSuccess;

  /// No description provided for @configResetFailed.
  ///
  /// In en, this message translates to:
  /// **'Failed to reset configuration: {error}'**
  String configResetFailed(Object error);

  /// No description provided for @logClearTooltip.
  ///
  /// In en, this message translates to:
  /// **'Clear Logs'**
  String get logClearTooltip;

  /// No description provided for @logAutoRefresh.
  ///
  /// In en, this message translates to:
  /// **'Auto-refresh'**
  String get logAutoRefresh;

  /// No description provided for @logClearSuccess.
  ///
  /// In en, this message translates to:
  /// **'Logs have been cleared.'**
  String get logClearSuccess;

  /// No description provided for @logClearFailed.
  ///
  /// In en, this message translates to:
  /// **'Failed to clear logs: {error}'**
  String logClearFailed(Object error);

  /// No description provided for @notAvailable.
  ///
  /// In en, this message translates to:
  /// **'N/A'**
  String get notAvailable;

  /// No description provided for @sessionLabel.
  ///
  /// In en, this message translates to:
  /// **'Session'**
  String get sessionLabel;

  /// No description provided for @commandLabel.
  ///
  /// In en, this message translates to:
  /// **'Command'**
  String get commandLabel;

  /// No description provided for @httpLabel.
  ///
  /// In en, this message translates to:
  /// **'HTTP'**
  String get httpLabel;

  /// No description provided for @ipLabel.
  ///
  /// In en, this message translates to:
  /// **'IP'**
  String get ipLabel;

  /// No description provided for @uidLabel.
  ///
  /// In en, this message translates to:
  /// **'UID'**
  String get uidLabel;

  /// No description provided for @durationLabel.
  ///
  /// In en, this message translates to:
  /// **'Duration'**
  String get durationLabel;

  /// No description provided for @hourUnit.
  ///
  /// In en, this message translates to:
  /// **'h'**
  String get hourUnit;

  /// No description provided for @minuteUnit.
  ///
  /// In en, this message translates to:
  /// **'m'**
  String get minuteUnit;

  /// No description provided for @secondUnit.
  ///
  /// In en, this message translates to:
  /// **'s'**
  String get secondUnit;

  /// No description provided for @logNoInfo.
  ///
  /// In en, this message translates to:
  /// **'No log information available'**
  String get logNoInfo;

  /// No description provided for @logRefreshTooltip.
  ///
  /// In en, this message translates to:
  /// **'Refresh logs'**
  String get logRefreshTooltip;

  /// No description provided for @navDashboard.
  ///
  /// In en, this message translates to:
  /// **'Dashboard'**
  String get navDashboard;

  /// No description provided for @navConfig.
  ///
  /// In en, this message translates to:
  /// **'Server Config'**
  String get navConfig;

  /// No description provided for @navLogs.
  ///
  /// In en, this message translates to:
  /// **'Logs'**
  String get navLogs;

  /// No description provided for @navSettings.
  ///
  /// In en, this message translates to:
  /// **'Settings'**
  String get navSettings;

  /// No description provided for @aboutPermissionsMenu.
  ///
  /// In en, this message translates to:
  /// **'How to Set Permissions'**
  String get aboutPermissionsMenu;

  /// No description provided for @settingsLicense.
  ///
  /// In en, this message translates to:
  /// **'Open Source Licenses'**
  String get settingsLicense;

  /// No description provided for @settingsCheckForUpdates.
  ///
  /// In en, this message translates to:
  /// **'Check for Updates'**
  String get settingsCheckForUpdates;

  /// No description provided for @settingsAuthor.
  ///
  /// In en, this message translates to:
  /// **'Author: {authorName}'**
  String settingsAuthor(String authorName);

  /// No description provided for @settingsCopyright.
  ///
  /// In en, this message translates to:
  /// **'Copyright © 2024 {authorName}'**
  String settingsCopyright(String authorName);

  /// No description provided for @configResetConfirmTitle.
  ///
  /// In en, this message translates to:
  /// **'Reset Configuration'**
  String get configResetConfirmTitle;

  /// No description provided for @configResetConfirmContent.
  ///
  /// In en, this message translates to:
  /// **'Are you sure you want to reset the configuration to default? This action cannot be undone.'**
  String get configResetConfirmContent;

  /// No description provided for @cancel.
  ///
  /// In en, this message translates to:
  /// **'Cancel'**
  String get cancel;

  /// No description provided for @confirm.
  ///
  /// In en, this message translates to:
  /// **'Confirm'**
  String get confirm;

  /// No description provided for @startupOnBoot.
  ///
  /// In en, this message translates to:
  /// **'Start on system boot'**
  String get startupOnBoot;

  /// No description provided for @startupOnBootSuccess.
  ///
  /// In en, this message translates to:
  /// **'Auto-start setting updated successfully'**
  String get startupOnBootSuccess;

  /// Message shown when failing to update auto-start setting
  ///
  /// In en, this message translates to:
  /// **'Failed to update auto-start setting: {error}'**
  String startupOnBootFailed(Object error);

  /// No description provided for @ok.
  ///
  /// In en, this message translates to:
  /// **'OK'**
  String get ok;

  /// No description provided for @connectionError.
  ///
  /// In en, this message translates to:
  /// **'Connection Error'**
  String get connectionError;

  /// No description provided for @info.
  ///
  /// In en, this message translates to:
  /// **'Info'**
  String get info;

  /// No description provided for @trayShowMainWindow.
  ///
  /// In en, this message translates to:
  /// **'Show Main Window'**
  String get trayShowMainWindow;

  /// No description provided for @trayExit.
  ///
  /// In en, this message translates to:
  /// **'Exit'**
  String get trayExit;

  /// No description provided for @dashboardUpdateAvailable.
  ///
  /// In en, this message translates to:
  /// **'A new version {version} is available.'**
  String dashboardUpdateAvailable(String version);

  /// No description provided for @dashboardUpdateButton.
  ///
  /// In en, this message translates to:
  /// **'Download'**
  String get dashboardUpdateButton;

  /// No description provided for @dashboardUpdateError.
  ///
  /// In en, this message translates to:
  /// **'Failed to check updates.'**
  String get dashboardUpdateError;

  /// No description provided for @hideOnStartup.
  ///
  /// In en, this message translates to:
  /// **'Hide main window on startup'**
  String get hideOnStartup;

  /// No description provided for @hideOnAutoStart.
  ///
  /// In en, this message translates to:
  /// **'Hide main window when auto-started on boot'**
  String get hideOnAutoStart;

  /// No description provided for @settingStartMinimized.
  ///
  /// In en, this message translates to:
  /// **'Start minimized to tray'**
  String get settingStartMinimized;

  /// No description provided for @settingStartMinimizedSuccess.
  ///
  /// In en, this message translates to:
  /// **'Start minimized setting updated successfully'**
  String get settingStartMinimizedSuccess;

  /// Message shown when failing to update start minimized setting
  ///
  /// In en, this message translates to:
  /// **'Failed to update start minimized setting: {error}'**
  String settingStartMinimizedFailed(Object error);

  /// No description provided for @linuxPermissionsTitle.
  ///
  /// In en, this message translates to:
  /// **'Permissions Setup Required'**
  String get linuxPermissionsTitle;

  /// No description provided for @linuxPermissionsOnlyLinux.
  ///
  /// In en, this message translates to:
  /// **'This page is only available on Linux'**
  String get linuxPermissionsOnlyLinux;

  /// No description provided for @linuxPermissionsSetupTitle.
  ///
  /// In en, this message translates to:
  /// **'Setup Instructions'**
  String get linuxPermissionsSetupTitle;

  /// No description provided for @linuxPermissionsStep1.
  ///
  /// In en, this message translates to:
  /// **'1. Add your user to the input group:'**
  String get linuxPermissionsStep1;

  /// No description provided for @linuxPermissionsStep2.
  ///
  /// In en, this message translates to:
  /// **'2. Copy the udev rule content below (you will paste it into the file created in Step 3):'**
  String get linuxPermissionsStep2;

  /// No description provided for @linuxPermissionsStep3.
  ///
  /// In en, this message translates to:
  /// **'3. Create (or open) the udev rule file, then paste the Step 2 content into it:'**
  String get linuxPermissionsStep3;

  /// No description provided for @linuxPermissionsStep4.
  ///
  /// In en, this message translates to:
  /// **'4. Reload udev rules:'**
  String get linuxPermissionsStep4;

  /// No description provided for @linuxPermissionsStep5.
  ///
  /// In en, this message translates to:
  /// **'5. Enable the uinput kernel module (load now and configure auto-load on boot):'**
  String get linuxPermissionsStep5;

  /// No description provided for @linuxPermissionsStep6.
  ///
  /// In en, this message translates to:
  /// **'6. Log out and back in (or reboot), launch MouseHero.'**
  String get linuxPermissionsStep6;

  /// No description provided for @linuxPermissionsHelp.
  ///
  /// In en, this message translates to:
  /// **'Help'**
  String get linuxPermissionsHelp;

  /// No description provided for @linuxPermissionsCopied.
  ///
  /// In en, this message translates to:
  /// **'Command copied to clipboard'**
  String get linuxPermissionsCopied;

  /// No description provided for @linuxPermissionsRuleCopied.
  ///
  /// In en, this message translates to:
  /// **'Rule copied to clipboard'**
  String get linuxPermissionsRuleCopied;
}

class _AppLocalizationsDelegate
    extends LocalizationsDelegate<AppLocalizations> {
  const _AppLocalizationsDelegate();

  @override
  Future<AppLocalizations> load(Locale locale) {
    return SynchronousFuture<AppLocalizations>(lookupAppLocalizations(locale));
  }

  @override
  bool isSupported(Locale locale) =>
      <String>['en', 'zh'].contains(locale.languageCode);

  @override
  bool shouldReload(_AppLocalizationsDelegate old) => false;
}

AppLocalizations lookupAppLocalizations(Locale locale) {
  // Lookup logic when only language code is specified.
  switch (locale.languageCode) {
    case 'en':
      return AppLocalizationsEn();
    case 'zh':
      return AppLocalizationsZh();
  }

  throw FlutterError(
    'AppLocalizations.delegate failed to load unsupported locale "$locale". This is likely '
    'an issue with the localizations generation tool. Please file an issue '
    'on GitHub with a reproducible sample app and the gen-l10n configuration '
    'that was used.',
  );
}
