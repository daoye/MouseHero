import 'package:fluent_ui/fluent_ui.dart';
import 'package:flutter/services.dart' show rootBundle;
import 'package:server_gui/l10n/app_localizations.dart';
import 'package:server_gui/services/config_service.dart';
import 'package:server_gui/services/server_process_service.dart';

class ConfigPage extends StatefulWidget {
  const ConfigPage({super.key});

  @override
  State<ConfigPage> createState() => _ConfigPageState();
}

class _ConfigPageState extends State<ConfigPage> {
  final ConfigService _configService = ConfigService();
  final TextEditingController _textController = TextEditingController();
  String? _configFilePath;

  @override
  void initState() {
    super.initState();
    _loadConfig();
  }

  @override
  void dispose() {
    _textController.dispose();
    super.dispose();
  }

  Future<void> _loadConfig() async {
    try {
      final configFile = await _configService.getConfigFile();
      final configContent = await _configService.readConfigFile();
      if (mounted) {
        setState(() {
          _configFilePath = configFile;
          _textController.text = configContent;
        });
      }
    } catch (e) {
      if (mounted) {
        setState(() {
          _configFilePath = AppLocalizations.of(
            context,
          )!.configLoadFailed(e.toString());
        });
      }
    }
  }

  Future<void> _resetConfig() async {
    final l10n = AppLocalizations.of(context)!;
    final confirmed = await showDialog<bool>(
      context: context,
      builder:
          (context) => ContentDialog(
            title: Text(l10n.configResetConfirmTitle),
            content: Text(l10n.configResetConfirmContent),
            actions: [
              Button(
                child: Text(l10n.cancel),
                onPressed: () => Navigator.pop(context, false),
              ),
              FilledButton(
                child: Text(l10n.confirm),
                onPressed: () => Navigator.pop(context, true),
              ),
            ],
          ),
    );

    if (confirmed != true) return;

    try {
      final defaultConfig = await rootBundle.loadString('assets/default.conf');
      if (mounted) {
        _textController.text = defaultConfig;
      }
      // _showDialog(l10n.configResetSuccess);
    } catch (e) {
      _showDialog(l10n.configResetFailed(e.toString()));
    }
  }

  Future<void> _saveConfig() async {
    final l10n = AppLocalizations.of(context)!;
    try {
      await _configService.writeConfigFile(_textController.text);

      if (ServerProcessService().isRunning) {
        await ServerProcessService().restartServer();
      }
      _showDialog(l10n.configSaveSuccess);
    } catch (e) {
      _showDialog(l10n.configSaveFailed(e.toString()));
    }
  }

  void _showDialog(String message) {
    final l10n = AppLocalizations.of(context)!;

    showDialog(
      context: context,
      builder:
          (context) => ContentDialog(
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
    final theme = FluentTheme.of(context);

    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Text(_configFilePath ?? '', style: theme.typography.caption),
        const SizedBox(height: 8),
        Expanded(
          child: TextBox(
            controller: _textController,
            maxLines: null,
            expands: true,
            placeholder: l10n.configLabel,
            style: const TextStyle(
              fontFamily: 'Consolas, Courier New, monospace',
            ),
          ),
        ),
        const SizedBox(height: 16),
        Row(
          mainAxisAlignment: MainAxisAlignment.end,
          children: [
            Button(
              onPressed: _resetConfig,
              child: Row(
                children: [
                  const Icon(FluentIcons.reset, size: 16),
                  const SizedBox(width: 6),
                  Text(l10n.configReset),
                ],
              ),
            ),
            const SizedBox(width: 8),
            FilledButton(
              onPressed: _saveConfig,
              child: Row(
                children: [
                  const Icon(FluentIcons.save, size: 16),
                  const SizedBox(width: 6),
                  Text(l10n.configSave),
                ],
              ),
            ),
          ],
        ),
      ],
    );
  }
}
