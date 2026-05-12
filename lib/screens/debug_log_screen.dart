import 'package:flutter/material.dart';
import 'package:flutter/services.dart';

import '../debug_log.dart';

class DebugLogScreen extends StatelessWidget {
  const DebugLogScreen({super.key, this.debugLog});

  final DebugLog? debugLog;

  DebugLog get _log => debugLog ?? DebugLog.instance;

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Debug log'),
        actions: [
          TextButton.icon(
            onPressed: () => _copyAll(context),
            icon: const Icon(Icons.copy),
            label: const Text('Копировать всё'),
          ),
          TextButton.icon(
            onPressed: _log.clear,
            icon: const Icon(Icons.delete_outline),
            label: const Text('Очистить'),
          ),
        ],
      ),
      body: StreamBuilder<List<DebugLogEntry>>(
        stream: _log.stream,
        initialData: _log.current,
        builder: (context, snapshot) {
          final entries = snapshot.data ?? const <DebugLogEntry>[];
          if (entries.isEmpty) {
            return const Center(child: Text('Лог пуст'));
          }
          return SingleChildScrollView(
            padding: const EdgeInsets.all(12),
            child: SelectableText(
              _formatEntries(entries),
              style: Theme.of(
                context,
              ).textTheme.bodySmall?.copyWith(fontFamily: 'monospace'),
            ),
          );
        },
      ),
    );
  }

  Future<void> _copyAll(BuildContext context) async {
    await Clipboard.setData(ClipboardData(text: _formatEntries(_log.current)));
    if (!context.mounted) {
      return;
    }
    ScaffoldMessenger.of(
      context,
    ).showSnackBar(const SnackBar(content: Text('Debug log скопирован')));
  }

  String _formatEntries(List<DebugLogEntry> entries) => entries
      .map((entry) => '[${entry.timeLabel}] ${entry.message}')
      .join('\n');
}
