import 'package:shared_preferences/shared_preferences.dart';

final class ConnectionHistory {
  ConnectionHistory._();

  static final ConnectionHistory instance = ConnectionHistory._();
  static const String _historyKey = 'connection_history';
  static const int _limit = 10;

  Future<List<String>> load() async {
    final preferences = await SharedPreferences.getInstance();
    return preferences.getStringList(_historyKey) ?? <String>[];
  }

  Future<List<String>> remember(String ipAddress) async {
    final current = await load();
    final next = <String>[
      ipAddress,
      ...current.where((entry) => entry != ipAddress),
    ].take(_limit).toList();
    final preferences = await SharedPreferences.getInstance();
    await preferences.setStringList(_historyKey, next);
    return next;
  }
}
