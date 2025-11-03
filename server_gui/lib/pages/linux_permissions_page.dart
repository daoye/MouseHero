import 'dart:io';
import 'package:fluent_ui/fluent_ui.dart';
import 'package:flutter/services.dart';
import 'package:flutter_markdown/flutter_markdown.dart';
import 'package:server_gui/l10n/app_localizations.dart';
import 'package:server_gui/services/linux_permissions_service.dart';

class LinuxPermissionsPage extends StatefulWidget {
  const LinuxPermissionsPage({super.key});

  @override
  State<LinuxPermissionsPage> createState() => _LinuxPermissionsPageState();
}

class _LinuxPermissionsPageState extends State<LinuxPermissionsPage> {
  final LinuxPermissionsService _permissionsService = LinuxPermissionsService();
  String? _helpContent;

  @override
  void initState() {
    super.initState();
    _loadHelpContent();
  }

  Future<void> _loadHelpContent() async {
    try {
      final content = await rootBundle.loadString('lib/help/udev_setup.md');
      if (mounted) {
        setState(() {
          _helpContent = content;
        });
      }
    } catch (e) {
      // Help content not available
    }
  }

  void _copyToClipboard(String text, bool isRule) {
    final l10n = AppLocalizations.of(context)!;
    Clipboard.setData(ClipboardData(text: text));
    displayInfoBar(
      context,
      builder: (context, close) {
        return InfoBar(
          title: Text(
            isRule
                ? l10n.linuxPermissionsRuleCopied
                : l10n.linuxPermissionsCopied,
          ),
          severity: InfoBarSeverity.success,
        );
      },
    );
  }

  Widget _buildSetupInstructions() {
    final l10n = AppLocalizations.of(context)!;
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              l10n.linuxPermissionsSetupTitle,
              style: FluentTheme.of(context).typography.subtitle,
            ),
            const SizedBox(height: 12),
            Text(l10n.linuxPermissionsStep1),
            const SizedBox(height: 8),
            Container(
              padding: const EdgeInsets.all(12),
              decoration: BoxDecoration(
                color: Colors.grey[190],
                borderRadius: BorderRadius.circular(4),
              ),
              child: Row(
                children: [
                  Expanded(
                    child: SelectableText(
                      'sudo usermod -aG ${LinuxPermissionsService.groupName} \$USER',
                      style: const TextStyle(fontFamily: 'monospace'),
                    ),
                  ),
                  IconButton(
                    icon: const Icon(FluentIcons.copy),
                    onPressed:
                        () => _copyToClipboard(
                          'sudo usermod -aG ${LinuxPermissionsService.groupName} \$USER',
                          false,
                        ),
                  ),
                ],
              ),
            ),
            const SizedBox(height: 12),
            Text(l10n.linuxPermissionsStep2),
            const SizedBox(height: 8),
            Container(
              padding: const EdgeInsets.all(12),
              decoration: BoxDecoration(
                color: Colors.grey[190],
                borderRadius: BorderRadius.circular(4),
              ),
              child: Row(
                children: [
                  Expanded(
                    child: SelectableText(
                      _permissionsService.getUdevRuleContent(),
                      style: const TextStyle(fontFamily: 'monospace'),
                    ),
                  ),
                  IconButton(
                    icon: const Icon(FluentIcons.copy),
                    onPressed:
                        () => _copyToClipboard(
                          _permissionsService.getUdevRuleContent(),
                          true,
                        ),
                  ),
                ],
              ),
            ),
            const SizedBox(height: 12),
            Text(l10n.linuxPermissionsStep3),
            const SizedBox(height: 8),
            Container(
              padding: const EdgeInsets.all(12),
              decoration: BoxDecoration(
                color: Colors.grey[190],
                borderRadius: BorderRadius.circular(4),
              ),
              child: Row(
                children: [
                  Expanded(
                    child: SelectableText(
                      'sudo nano ${_permissionsService.getUdevRulePath()}',
                      style: const TextStyle(fontFamily: 'monospace'),
                    ),
                  ),
                  IconButton(
                    icon: const Icon(FluentIcons.copy),
                    onPressed:
                        () => _copyToClipboard(
                          'sudo nano ${_permissionsService.getUdevRulePath()}',
                          false,
                        ),
                  ),
                ],
              ),
            ),
            const SizedBox(height: 12),
            Text(l10n.linuxPermissionsStep4),
            const SizedBox(height: 8),
            Container(
              padding: const EdgeInsets.all(12),
              decoration: BoxDecoration(
                color: Colors.grey[190],
                borderRadius: BorderRadius.circular(4),
              ),
              child: Row(
                children: [
                  const Expanded(
                    child: SelectableText(
                      'sudo udevadm control --reload-rules && sudo udevadm trigger',
                      style: TextStyle(fontFamily: 'monospace'),
                    ),
                  ),
                  IconButton(
                    icon: const Icon(FluentIcons.copy),
                    onPressed:
                        () => _copyToClipboard(
                          'sudo udevadm control --reload-rules && sudo udevadm trigger',
                          false,
                        ),
                  ),
                ],
              ),
            ),
            const SizedBox(height: 12),
            Text(l10n.linuxPermissionsStep5),
            const SizedBox(height: 8),
            Container(
              padding: const EdgeInsets.all(12),
              decoration: BoxDecoration(
                color: Colors.grey[190],
                borderRadius: BorderRadius.circular(4),
              ),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Row(
                    children: [
                      const Expanded(
                        child: SelectableText(
                          'sudo modprobe uinput',
                          style: TextStyle(fontFamily: 'monospace'),
                        ),
                      ),
                      IconButton(
                        icon: const Icon(FluentIcons.copy),
                        onPressed:
                            () =>
                                _copyToClipboard('sudo modprobe uinput', false),
                      ),
                    ],
                  ),
                  const SizedBox(height: 8),
                  Row(
                    children: [
                      const Expanded(
                        child: SelectableText(
                          'echo uinput | sudo tee /etc/modules-load.d/mousehero-uinput.conf',
                          style: TextStyle(fontFamily: 'monospace'),
                        ),
                      ),
                      IconButton(
                        icon: const Icon(FluentIcons.copy),
                        onPressed:
                            () => _copyToClipboard(
                              'echo uinput | sudo tee /etc/modules-load.d/mousehero-uinput.conf',
                              false,
                            ),
                      ),
                    ],
                  ),
                ],
              ),
            ),
            const SizedBox(height: 12),
            Text(l10n.linuxPermissionsStep6),
          ],
        ),
      ),
    );
  }

  Widget _buildHelpSection() {
    final l10n = AppLocalizations.of(context)!;
    return Expander(
      leading: const Icon(FluentIcons.help),
      header: Text(l10n.linuxPermissionsHelp),
      initiallyExpanded: false,
      content: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          const SizedBox(height: 8),
          MarkdownBody(
            data: _helpContent!,
            selectable: true,
            styleSheet: MarkdownStyleSheet(
              h1: FluentTheme.of(context).typography.title,
              h2: FluentTheme.of(context).typography.subtitle,
              h3: FluentTheme.of(context).typography.bodyLarge,
              p: FluentTheme.of(context).typography.body,
              code: TextStyle(
                fontFamily: 'monospace',
                backgroundColor: Colors.grey[190],
              ),
              codeblockDecoration: BoxDecoration(
                color: Colors.grey[190],
                borderRadius: BorderRadius.circular(4),
              ),
            ),
          ),
        ],
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    final l10n = AppLocalizations.of(context)!;
    if (!Platform.isLinux) {
      return ScaffoldPage(
        content: Center(child: Text(l10n.linuxPermissionsOnlyLinux)),
      );
    }

    return ScaffoldPage(
      header: PageHeader(
        leading: Padding(
          padding: EdgeInsetsGeometry.symmetric(horizontal: 20),
          child: IconButton(
            icon: const Icon(FluentIcons.back, size: 18),
            onPressed: () => Navigator.of(context).maybePop(),
          ),
        ),
        title: Text(l10n.aboutPermissionsMenu),
      ),
      content: SingleChildScrollView(
        padding: const EdgeInsets.all(16.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            const SizedBox(height: 16),
            _buildSetupInstructions(),
            const SizedBox(height: 16),
            if (_helpContent != null) _buildHelpSection(),
          ],
        ),
      ),
    );
  }
}
