import 'dart:async';
import 'package:fluent_ui/fluent_ui.dart';
import 'package:provider/provider.dart';
import 'package:server_gui/l10n/app_localizations.dart';
import 'package:server_gui/providers/settings_provider.dart';
import 'package:server_gui/services/log_service.dart';

class LogPage extends StatefulWidget {
  const LogPage({super.key});

  @override
  State<LogPage> createState() => _LogPageState();
}

class _LogPageState extends State<LogPage> {
  final LogService _logService = LogService();
  final ScrollController _scrollController = ScrollController();
  String _logs = '';
  Timer? _timer;

  @override
  void initState() {
    super.initState();
    _fetchLogs();
    final settingsProvider = Provider.of<SettingsProvider>(context, listen: false);
    if (settingsProvider.logAutoRefresh) {
      _startTimer();
    }
  }

  @override
  void dispose() {
    _timer?.cancel();
    _scrollController.dispose();
    super.dispose();
  }

  void _scrollToBottom() {
    if (!mounted) return;
    WidgetsBinding.instance.addPostFrameCallback((_) {
      if (_scrollController.hasClients) {
        _scrollController.jumpTo(_scrollController.position.maxScrollExtent);
      }
    });
  }

  void _startTimer() {
    _timer?.cancel(); // Cancel any existing timer
    _timer = Timer.periodic(const Duration(seconds: 2), (timer) {
      _fetchLogs();
    });
  }

  Future<void> _fetchLogs() async {
    if (!mounted) return;
    try {
      final logs = await _logService.getLogs();
      if (mounted) {
        setState(() {
          _logs = logs;
        });
        _scrollToBottom();
      }
    } catch (e) {
      if (mounted) {
        setState(() {
          final l10n = AppLocalizations.of(context)!;
          _logs = '${l10n.logLoadingError}: ${e.toString()}';
        });
      }
    }
  }

  Future<void> _clearLogs() async {
    await _logService.clearLogs();
    await _fetchLogs();
  }

  void _onAutoRefreshChanged(bool value) {
    final settingsProvider = Provider.of<SettingsProvider>(context, listen: false);
    settingsProvider.setLogAutoRefresh(value);
    
    if (value) {
      _startTimer();
    } else {
      _timer?.cancel();
    }
  }

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context)!;

    return Column(
      children: [
        Expanded(
          child: Padding(
            padding: const EdgeInsets.only(bottom: 16),
            child:
                _logs.isEmpty
                    ? Center(child: Text(l10n.logNoRecords))
                    : TextBox(
                      controller: TextEditingController(text: _logs),
                      maxLines: null,
                      expands: true,
                      readOnly: true,
                      scrollController: _scrollController,
                      style: const TextStyle(
                        fontFamily: 'Consolas, Courier New, monospace',
                      ),
                    ),
          ),
        ),
        Row(
          mainAxisAlignment: MainAxisAlignment.spaceBetween,
          children: [
            Button(
              onPressed: _clearLogs,
              child: Row(
                children: [
                  const Icon(FluentIcons.delete),
                  const SizedBox(width: 6),
                  Text(l10n.logClearTooltip),
                ],
              ),
            ),
            Row(
              children: [
                Text(l10n.logAutoRefresh),
                const SizedBox(width: 8),
                Consumer<SettingsProvider>(
                  builder: (context, settingsProvider, _) => ToggleSwitch(
                    checked: settingsProvider.logAutoRefresh,
                    onChanged: _onAutoRefreshChanged,
                  ),
                ),
                const SizedBox(width: 8),
                Button(
                  onPressed: _fetchLogs,
                  child: Row(
                    children: [
                      const Icon(FluentIcons.refresh),
                      const SizedBox(width: 6),
                      Text(l10n.logRefreshTooltip),
                    ],
                  ),
                ),
              ],
            ),
          ],
        ),
      ],
    );
  }
}
