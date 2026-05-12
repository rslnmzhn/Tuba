import 'dart:async';

final class DebugLogEntry {
  const DebugLogEntry({required this.timestamp, required this.message});

  final DateTime timestamp;
  final String message;

  String get timeLabel {
    String two(int value) => value.toString().padLeft(2, '0');
    String three(int value) => value.toString().padLeft(3, '0');
    return '${two(timestamp.hour)}:${two(timestamp.minute)}:'
        '${two(timestamp.second)}.${three(timestamp.millisecond)}';
  }
}

final class DebugLog {
  DebugLog._();

  static final DebugLog instance = DebugLog._();
  static const int _maxEntries = 300;

  final List<DebugLogEntry> _entries = <DebugLogEntry>[];
  final StreamController<List<DebugLogEntry>> _controller =
      StreamController<List<DebugLogEntry>>.broadcast();

  Stream<List<DebugLogEntry>> get stream => _controller.stream;

  List<DebugLogEntry> get current => List<DebugLogEntry>.unmodifiable(_entries);

  void add(String message) {
    _entries.add(DebugLogEntry(timestamp: DateTime.now(), message: message));
    if (_entries.length > _maxEntries) {
      _entries.removeRange(0, _entries.length - _maxEntries);
    }
    _controller.add(current);
  }

  void clear() {
    _entries.clear();
    _controller.add(current);
  }
}
