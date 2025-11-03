// ignore: unused_import
import 'package:intl/intl.dart' as intl;
import 'app_localizations.dart';

// ignore_for_file: type=lint

/// The translations for Chinese (`zh`).
class AppLocalizationsZh extends AppLocalizations {
  AppLocalizationsZh([String locale = 'zh']) : super(locale);

  @override
  String get appTitle => '鼠标侠';

  @override
  String get status => '状态';

  @override
  String get running => '运行中';

  @override
  String get stopped => '已停止';

  @override
  String get address => '地址';

  @override
  String get stopServer => '停止服务';

  @override
  String get startServer => '启动服务';

  @override
  String get scanToConnect => '请使用 鼠标侠 客户端扫描二维码连接';

  @override
  String get logs => '实时日志';

  @override
  String clients(Object count) {
    return '客户端 ($count)';
  }

  @override
  String get settings => '配置';

  @override
  String get themeMode => '主题模式';

  @override
  String get themeSystem => '跟随系统';

  @override
  String get themeLight => '浅色模式';

  @override
  String get themeDark => '深色模式';

  @override
  String get language => '语言';

  @override
  String get languageEnglish => '英文';

  @override
  String get languageChinese => '中文';

  @override
  String get about => '关于 鼠标侠';

  @override
  String get loading => '加载中...';

  @override
  String get dashboardServerStatus => '服务端状态';

  @override
  String get dashboardScanToConnect => '扫码连接';

  @override
  String get dashboardServiceControl => '服务控制';

  @override
  String dashboardOnlineClients(Object count) {
    return '在线客户端 ($count)';
  }

  @override
  String get dashboardStatusConnected => '已连接';

  @override
  String get dashboardStatusNotConnected => '未连接';

  @override
  String get dashboardStatusNoData => '暂无数据';

  @override
  String get dashboardStatusRunning => '运行中';

  @override
  String get dashboardStatusStopped => '已停止';

  @override
  String get dashboardButtonStop => '停止';

  @override
  String get dashboardButtonRestart => '重启';

  @override
  String get dashboardButtonStart => '启动服务';

  @override
  String get starting => '启动中...';

  @override
  String get dashboardActionSuccessStop => '服务已成功停止';

  @override
  String get dashboardActionSuccessRestart => '服务重启成功。';

  @override
  String get dashboardActionSuccessStart => '服务已成功启动。';

  @override
  String dashboardActionFailed(Object error) {
    return '操作失败：$error';
  }

  @override
  String get dashboardStatusError => '加载仪表盘时出错';

  @override
  String get retry => '重试';

  @override
  String get dashboardNoClients => '暂无在线客户端';

  @override
  String configLoadFailed(Object error) {
    return '加载配置失败：$error';
  }

  @override
  String get configSaveSuccess => '配置已成功保存！';

  @override
  String configSaveFailed(Object error) {
    return '保存配置失败：$error';
  }

  @override
  String get configReset => '重置';

  @override
  String get configSave => '保存更改';

  @override
  String get configLabel => '服务端配置 (server.conf)';

  @override
  String get logPageTitle => '日志';

  @override
  String get logLoadingError => '加载日志失败';

  @override
  String get logTitle => '日志';

  @override
  String get logNoRecords => '暂无日志记录';

  @override
  String get reset => '重置';

  @override
  String get saveChanges => '保存更改';

  @override
  String get configResetSuccess => '配置已重置为默认值。';

  @override
  String configResetFailed(Object error) {
    return '重置配置失败: $error';
  }

  @override
  String get logClearTooltip => '清空日志';

  @override
  String get logAutoRefresh => '自动刷新';

  @override
  String get logClearSuccess => '日志已清空。';

  @override
  String logClearFailed(Object error) {
    return '清空日志失败: $error';
  }

  @override
  String get notAvailable => '不可用';

  @override
  String get sessionLabel => '会话';

  @override
  String get commandLabel => '命令';

  @override
  String get httpLabel => 'HTTP';

  @override
  String get ipLabel => 'IP';

  @override
  String get uidLabel => '用户ID';

  @override
  String get durationLabel => '时长';

  @override
  String get hourUnit => '小时';

  @override
  String get minuteUnit => '分';

  @override
  String get secondUnit => '秒';

  @override
  String get logNoInfo => '没有可用的日志信息';

  @override
  String get logRefreshTooltip => '刷新日志';

  @override
  String get navDashboard => '概览';

  @override
  String get navConfig => '服务端配置';

  @override
  String get navLogs => '日志';

  @override
  String get navSettings => '设置';

  @override
  String get aboutPermissionsMenu => '如何设置权限';

  @override
  String get settingsLicense => '开源协议';

  @override
  String get settingsCheckForUpdates => '检查更新';

  @override
  String settingsAuthor(String authorName) {
    return '作者: $authorName';
  }

  @override
  String settingsCopyright(String authorName) {
    return '版权所有 © 2024 $authorName';
  }

  @override
  String get configResetConfirmTitle => '重置配置';

  @override
  String get configResetConfirmContent => '确定要将配置重置为默认值吗？此操作无法撤销。';

  @override
  String get cancel => '取消';

  @override
  String get confirm => '确认';

  @override
  String get startupOnBoot => '开机自动启动';

  @override
  String get startupOnBootSuccess => '开机启动设置已成功更新';

  @override
  String startupOnBootFailed(Object error) {
    return '更新开机启动设置失败: $error';
  }

  @override
  String get ok => '好的';

  @override
  String get connectionError => '连接错误';

  @override
  String get info => '信息';

  @override
  String get trayShowMainWindow => '显示主界面';

  @override
  String get trayExit => '退出';

  @override
  String dashboardUpdateAvailable(String version) {
    return '发现新版本 $version';
  }

  @override
  String get dashboardUpdateButton => '立即前往';

  @override
  String get dashboardUpdateError => '检查更新失败';

  @override
  String get hideOnStartup => '启动时隐藏主窗口';

  @override
  String get hideOnAutoStart => '开机自启时隐藏主界面';

  @override
  String get settingStartMinimized => '启动时隐藏主界面';

  @override
  String get settingStartMinimizedSuccess => '隐藏主界面设置已成功更新';

  @override
  String settingStartMinimizedFailed(Object error) {
    return '更新隐藏主界面设置失败: $error';
  }

  @override
  String get linuxPermissionsTitle => '需要设置权限';

  @override
  String get linuxPermissionsOnlyLinux => '此页面仅适用于 Linux 系统';

  @override
  String get linuxPermissionsSetupTitle => '设置步骤';

  @override
  String get linuxPermissionsStep1 => '1. 将当前用户添加到 input 用户组：';

  @override
  String get linuxPermissionsStep2 => '2. 复制下面的 udev 规则内容（你将在步骤 3 创建的文件中粘贴它）：';

  @override
  String get linuxPermissionsStep3 => '3. 创建（或打开）udev 规则文件，然后将步骤 2 的内容粘贴进去：';

  @override
  String get linuxPermissionsStep4 => '4. 重新加载 udev 规则：';

  @override
  String get linuxPermissionsStep5 => '5. 启用 uinput 内核模块（立即加载并设置开机自动加载）：';

  @override
  String get linuxPermissionsStep6 => '6. 注销并重新登录（或重启），启动 MouseHero。';

  @override
  String get linuxPermissionsHelp => '帮助';

  @override
  String get linuxPermissionsCopied => '命令已复制到剪贴板';

  @override
  String get linuxPermissionsRuleCopied => '规则已复制到剪贴板';
}
