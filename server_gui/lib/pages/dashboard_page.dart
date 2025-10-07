import 'dart:async';

import 'package:fluent_ui/fluent_ui.dart';
import 'package:package_info_plus/package_info_plus.dart';
import 'package:qr_flutter/qr_flutter.dart';
import 'package:server_gui/l10n/app_localizations.dart';
import 'package:server_gui/services/api_service.dart';
import 'package:server_gui/services/config_service.dart';
import 'package:server_gui/services/network_service.dart';
import 'package:server_gui/services/server_process_service.dart';
import 'package:server_gui/services/update_service.dart';
import 'package:url_launcher/url_launcher.dart';

class DashboardPage extends StatefulWidget {
  const DashboardPage({super.key});

  @override
  State<DashboardPage> createState() => _DashboardPageState();
}

class _DashboardPageState extends State<DashboardPage> {
  Map<String, dynamic> status = {'clients': []};
  final ApiService _apiService = ApiService();
  final ConfigService _configService = ConfigService();
  final ServerProcessService _serverProcessService = ServerProcessService();
  final UpdateService _updateService = UpdateService();
  Timer? _refreshTimer;
  StreamSubscription? _runningStateSubscription;

  bool _waitting = false;
  bool _isRunning = false;
  UpdateCheckResult? _updateResult;
  String? _updateError;

  String sessionAddr = 'unknow';
  String sessionPort = 'unknow';
  String commandAddr = 'unknow';
  String commandPort = 'unknow';

  String localIp = '';

  @override
  void initState() {
    super.initState();

    _isRunning = _serverProcessService.isRunning;

    _runningStateSubscription = _serverProcessService.stateStream.listen((
      isRunning,
    ) {
      if (mounted) {
        setState(() {
          _isRunning = isRunning;
          _waitting = false;
        });

        _fetchStatus();
      }
    });

    Future.microtask(() async {
      var h = await _configService.getValue('session', 'listener');
      sessionAddr = h?.split(':').first ?? 'unknow';
      sessionPort = h?.split(':').last ?? 'unknow';

      h = await _configService.getValue('command', 'listener');
      commandAddr = h?.split(':').first ?? 'unknow';
      commandPort = h?.split(':').last ?? 'unknow';

      localIp = await NetworkService().getPreferredIp();

      setState(() {});
    });

    Future.microtask(_checkForUpdates);

    _fetchStatus();

    _refreshTimer = Timer.periodic(
      const Duration(seconds: 3),
      (_) => _fetchStatus(),
    );
  }

  @override
  void dispose() {
    _refreshTimer?.cancel();
    _runningStateSubscription?.cancel();
    super.dispose();
  }

  Future<void> _checkForUpdates() async {
    try {
      final packageInfo = await PackageInfo.fromPlatform();
      final result = await _updateService.checkForUpdates(
        currentVersion: packageInfo.version,
        currentBuildNumber: packageInfo.buildNumber,
      );

      if (!mounted) return;
      setState(() {
        _updateResult = result;
        _updateError = result.hasError ? result.errorMessage : null;
      });
    } catch (e) {
      if (!mounted) return;
      setState(() {
        _updateError = e.toString();
      });
    }
  }

  Future<void> _openUpdateWebsite(BuildContext context) async {
    final uri = _updateResult?.releaseUrl ?? UpdateService.defaultReleaseUri;
    final l10n = AppLocalizations.of(context)!;
    final launched = await launchUrl(uri, mode: LaunchMode.externalApplication);
    if (!launched && mounted) {
      _showConnectionErrorDialog(l10n.dashboardUpdateError);
    }
  }

  Future _fetchStatus() async {
    Map<String, dynamic>? status;
    if (_serverProcessService.isRunning) {
      try {
        final String? origin = await _configService.getValue(
          'http',
          'listener',
        );

        status = await _apiService.getStatus(origin: origin);
      } catch (_) {}
    }

    if (mounted) {
      setState(() {
        this.status = status ?? {'clients': []};
      });
    }
  }

  Future<void> _handleControlAction(
    Future<void> Function() action,
    String Function(Object) errorMessageBuilder,
  ) async {
    setState(() {
      _waitting = true;
    });

    try {
      await action();
      _fetchStatus();
    } catch (e) {
      _showConnectionErrorDialog(errorMessageBuilder(e));
    }
  }

  void _showConnectionErrorDialog(String message) {
    final l10n = AppLocalizations.of(context)!;
    showDialog(
      context: context,
      builder:
          (context) => ContentDialog(
            title: Text(l10n.connectionError),
            content: Text(message),
            actions: [
              Button(
                child: Text(l10n.ok),
                onPressed: () => Navigator.pop(context),
              ),
            ],
          ),
    );
  }

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context)!;

    return _buildDashboardGrid(context, l10n);
  }

  Widget _buildDashboardGrid(BuildContext context, AppLocalizations l10n) {
    return Padding(
      padding: EdgeInsets.zero,
      child: Column(
        children: [
          if (_updateResult?.hasUpdate ?? false)
            Padding(
              padding: const EdgeInsets.only(bottom: 12),
              child: InfoBar(
                severity: InfoBarSeverity.success,
                title: Text(
                  l10n.dashboardUpdateAvailable(
                    _updateResult!.versionLabel ??
                        _updateResult!.latestVersion ??
                        '',
                  ),
                ),
                action: Button(
                  onPressed: () => _openUpdateWebsite(context),
                  child: Text(l10n.dashboardUpdateButton),
                ),
              ),
            ),
          Expanded(
            child: Row(
              children: [
                Expanded(child: _buildServerStatusCard(context, l10n)),
                const SizedBox(width: 12),
                Expanded(child: _buildScanCard(context, l10n)),
              ],
            ),
          ),
          const SizedBox(height: 12),
          Expanded(
            child: Row(
              children: [
                Expanded(child: _buildServiceControlCard(context, l10n)),
                const SizedBox(width: 12),
                Expanded(child: _buildOnlineClientsCard(context, l10n)),
              ],
            ),
          ),
        ],
      ),
    );
  }

  Widget _buildServerStatusCard(BuildContext context, AppLocalizations l10n) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            _buildCardTitle(context, l10n.dashboardServerStatus),
            const Spacer(),
            _buildInfoRow(
              context,
              _isRunning ? FluentIcons.accept : FluentIcons.cancel,
              l10n.status,
              _isRunning
                  ? l10n.dashboardStatusRunning
                  : l10n.dashboardStatusStopped,
            ),
            const SizedBox(height: 8),
            _buildInfoRow(
              context,
              FluentIcons.plug_connected,
              l10n.sessionLabel,
              "$sessionAddr:$sessionPort",
            ),
            const SizedBox(height: 8),
            _buildInfoRow(
              context,
              FluentIcons.command_prompt,
              l10n.commandLabel,
              "$commandAddr:$commandPort",
            ),
            const Spacer(),
          ],
        ),
      ),
    );
  }

  Widget _buildScanCard(BuildContext context, AppLocalizations l10n) {
    final ip =
        sessionAddr == "0.0.0.0" || sessionAddr == "::" ? localIp : sessionAddr;
    final port = sessionPort;
    final qrData = '$ip:$port';

    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.center,
          children: [
            _buildCardTitle(context, l10n.dashboardScanToConnect),
            const SizedBox(height: 10),
            Expanded(
              child: Center(
                child: QrImageView(
                  data: qrData,
                  version: QrVersions.auto,
                  backgroundColor: Colors.white,
                ),
              ),
            ),
            const SizedBox(height: 10),
            Text('${l10n.ipLabel}: $ip'),
          ],
        ),
      ),
    );
  }

  Widget _buildServiceControlCard(BuildContext context, AppLocalizations l10n) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          children: [
            _buildCardTitle(context, l10n.dashboardServiceControl),
            const Spacer(),
            if (_isRunning)
              Row(
                mainAxisAlignment: MainAxisAlignment.spaceEvenly,
                children: [
                  ConstrainedBox(
                    constraints: const BoxConstraints(minWidth: 120),
                    child: FilledButton(
                      child: Row(
                        mainAxisSize: MainAxisSize.min,
                        children: [
                          Icon(
                            FluentIcons.stop_solid,
                            color: Colors.warningSecondaryColor,
                          ),
                          const SizedBox(width: 8),
                          Text(l10n.dashboardButtonStop),
                        ],
                      ),
                      onPressed:
                          () => _handleControlAction(
                            _serverProcessService.stopServer,
                            (e) => l10n.dashboardActionFailed(e),
                          ),
                    ),
                  ),
                  ConstrainedBox(
                    constraints: const BoxConstraints(minWidth: 120),
                    child: Button(
                      child: Row(
                        mainAxisSize: MainAxisSize.min,
                        children: [
                          const Icon(FluentIcons.refresh),
                          const SizedBox(width: 8),
                          Text(l10n.dashboardButtonRestart),
                        ],
                      ),
                      onPressed:
                          () => _handleControlAction(
                            _serverProcessService.restartServer,
                            (e) => l10n.dashboardActionFailed(e),
                          ),
                    ),
                  ),
                ],
              )
            else
              ConstrainedBox(
                constraints: const BoxConstraints(minWidth: 120),
                child: FilledButton(
                  onPressed:
                      _waitting
                          ? null
                          : () async {
                            try {
                              await _serverProcessService.startServer();
                              _fetchStatus();
                            } catch (e) {
                              _showConnectionErrorDialog(
                                l10n.dashboardActionFailed(e),
                              );
                              setState(() {
                                _waitting = false;
                              });
                            }
                          },
                  child: Row(
                    mainAxisSize: MainAxisSize.min,
                    children: [
                      _waitting
                          ? const SizedBox(
                            width: 16,
                            height: 16,
                            child: ProgressRing(strokeWidth: 2),
                          )
                          : const Icon(FluentIcons.play),
                      const SizedBox(width: 8),
                      Text(
                        _waitting ? l10n.starting : l10n.dashboardButtonStart,
                      ),
                    ],
                  ),
                ),
              ),
            const Spacer(),
          ],
        ),
      ),
    );
  }

  Widget _buildOnlineClientsCard(BuildContext context, AppLocalizations l10n) {
    final List<dynamic> clients = status['clients'] as List<dynamic>? ?? [];

    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            _buildCardTitle(
              context,
              l10n.dashboardOnlineClients(clients.length),
            ),
            const SizedBox(height: 8),
            // 表格样式：表头 + 数据行
            if (clients.isEmpty)
              Expanded(child: Center(child: Text(l10n.dashboardNoClients)))
            else ...[
              // 表头
              Row(
                children: [
                  Expanded(
                    flex: 2,
                    child: Text(
                      l10n.uidLabel,
                      style: FluentTheme.of(context).typography.caption,
                    ),
                  ),
                  Expanded(
                    flex: 2,
                    child: Text(
                      l10n.durationLabel,
                      style: FluentTheme.of(context).typography.caption,
                    ),
                  ),
                  Expanded(
                    flex: 3,
                    child: Text(
                      l10n.ipLabel,
                      style: FluentTheme.of(context).typography.caption,
                    ),
                  ),
                ],
              ),
              const SizedBox(height: 8),
              // 数据区
              Expanded(
                child: ListView.separated(
                  itemCount: clients.length,
                  separatorBuilder: (context, index) => const SizedBox(height: 8),
                  itemBuilder: (context, index) {
                    final client = clients[index] as Map? ?? {};
                    final String uid =
                        client['uid']?.toString() ?? l10n.notAvailable;
                    final String ip =
                        client['ip']?.toString() ?? l10n.notAvailable;
                    final int? connectedAt =
                        (client['connected_at'] is int)
                            ? client['connected_at'] as int
                            : (client['connected_at'] is num)
                            ? (client['connected_at'] as num).toInt()
                            : null;

                    String formatDuration(int seconds) {
                      final d = Duration(seconds: seconds);
                      final h = d.inHours;
                      final m = d.inMinutes % 60;
                      final s = d.inSeconds % 60;
                      if (h > 0) return '$h${l10n.hourUnit} $m${l10n.minuteUnit} $s${l10n.secondUnit}';
                      if (m > 0) return '$m${l10n.minuteUnit} $s${l10n.secondUnit}';
                      return '$s${l10n.secondUnit}';
                    }

                    String durationText = l10n.notAvailable;
                    if (connectedAt != null && connectedAt > 0) {
                      final nowSec =
                          DateTime.now().millisecondsSinceEpoch ~/ 1000;
                      final diff = (nowSec - connectedAt);
                      if (diff >= 0) {
                        durationText = formatDuration(diff);
                      }
                    }

                    return Row(
                        children: [
                          Expanded(
                            flex: 2,
                            child: Text(
                              uid,
                              maxLines: 1,
                              overflow: TextOverflow.ellipsis,
                            ),
                          ),
                          Expanded(
                            flex: 2,
                            child: Text(
                              durationText,
                              maxLines: 1,
                              overflow: TextOverflow.ellipsis,
                              style: FluentTheme.of(context).typography.caption,
                            ),
                          ),
                          Expanded(
                            flex: 3,
                            child: Text(
                              ip,
                              maxLines: 1,
                              overflow: TextOverflow.ellipsis,
                              style: FluentTheme.of(context).typography.caption,
                            ),
                          ),
                        ],
                      );
                  },
                ),
              ),
            ],
          ],
        ),
      ),
    );
  }

  Widget _buildCardTitle(BuildContext context, String title) {
    final theme = FluentTheme.of(context);
    return Text(title, style: theme.typography.subtitle);
  }

  Widget _buildInfoRow(
    BuildContext context,
    IconData icon,
    String label,
    String value,
  ) {
    final bool isStatusRow = label == AppLocalizations.of(context)!.status;

    final Color? valueColor =
        isStatusRow ? (_isRunning ? Colors.green : Colors.red) : null;

    return Row(
      children: [
        Expanded(
          flex: 1,
          child: Row(
            mainAxisAlignment: MainAxisAlignment.end,
            children: [
              Icon(icon, size: 16),
              const SizedBox(width: 8),
              Text('$label:'),
            ],
          ),
        ),
        // 标题和值之间的间距为16px
        const SizedBox(width: 16),
        // 值左对齐
        Expanded(
          flex: 1,
          child: Text(value, style: TextStyle(color: valueColor)),
        ),
      ],
    );
  }
}
