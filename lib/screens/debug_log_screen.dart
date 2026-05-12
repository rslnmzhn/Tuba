import 'package:flutter/material.dart';

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
          return ListView.separated(
            padding: const EdgeInsets.all(12),
            itemCount: entries.length,
            separatorBuilder: (_, _) => const Divider(height: 12),
            itemBuilder: (context, index) {
              final entry = entries[index];
              return SelectableText(
                '[${entry.timeLabel}] ${entry.message}',
                style: Theme.of(
                  context,
                ).textTheme.bodySmall?.copyWith(fontFamily: 'monospace'),
              );
            },
          );
        },
      ),
    );
  }
}
