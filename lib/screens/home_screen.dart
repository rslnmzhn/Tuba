import 'package:flutter/material.dart';

import '../connection_history.dart';
import '../ffi/bridge.dart';
import 'viewer_screen.dart';

class HomeScreen extends StatefulWidget {
  const HomeScreen({super.key});

  @override
  State<HomeScreen> createState() => _HomeScreenState();
}

class _HomeScreenState extends State<HomeScreen> {
  final TextEditingController _controller = TextEditingController();
  final ConnectionHistory _connectionHistory = ConnectionHistory.instance;
  RcBridge get _bridge => RcBridge.instance;
  List<String> _history = <String>[];
  bool _connecting = false;

  @override
  void initState() {
    super.initState();
    _loadHistory();
  }

  @override
  void dispose() {
    _controller.dispose();
    super.dispose();
  }

  Future<void> _loadHistory() async {
    final history = await _connectionHistory.load();
    if (!mounted) {
      return;
    }
    setState(() {
      _history = history;
    });
  }

  Future<void> _saveHistory(String ipAddress) async {
    final nextHistory = await _connectionHistory.remember(ipAddress);
    if (mounted) {
      setState(() {
        _history = nextHistory;
      });
    }
  }

  Future<void> _connect([String? selectedIp]) async {
    final ipAddress = (selectedIp ?? _controller.text).trim();
    if (ipAddress.isEmpty || _connecting) {
      return;
    }

    setState(() {
      _connecting = true;
    });

    final result = _bridge.connect(ipAddress);
    if (!mounted) {
      return;
    }

    setState(() {
      _connecting = false;
    });

    if (result != 0) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('Не удалось подключиться: код $result')),
      );
      return;
    }

    await _saveHistory(ipAddress);

    if (!mounted) {
      return;
    }
    await Navigator.of(context).push(
      MaterialPageRoute<void>(
        builder: (context) => ViewerScreen(ipAddress: ipAddress),
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text('Tuba')),
      body: SafeArea(
        child: Padding(
          padding: const EdgeInsets.all(16),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.stretch,
            children: [
              TextField(
                controller: _controller,
                decoration: const InputDecoration(
                  border: OutlineInputBorder(),
                  labelText: 'IP адрес хоста',
                  hintText: '192.168.1.10',
                ),
                keyboardType: TextInputType.url,
                textInputAction: TextInputAction.go,
                onSubmitted: (_) => _connect(),
              ),
              const SizedBox(height: 12),
              FilledButton.icon(
                onPressed: _connecting ? null : () => _connect(),
                icon: _connecting
                    ? const SizedBox.square(
                        dimension: 18,
                        child: CircularProgressIndicator(strokeWidth: 2),
                      )
                    : const Icon(Icons.cast_connected),
                label: Text(_connecting ? 'Подключение...' : 'Подключиться'),
              ),
              const SizedBox(height: 24),
              Text('История', style: Theme.of(context).textTheme.titleMedium),
              const SizedBox(height: 8),
              Expanded(
                child: _history.isEmpty
                    ? const Center(child: Text('История подключений пуста'))
                    : ListView.separated(
                        itemCount: _history.length,
                        separatorBuilder: (_, _) => const Divider(height: 1),
                        itemBuilder: (context, index) {
                          final item = _history[index];
                          return ListTile(
                            leading: const Icon(Icons.computer),
                            title: Text(item),
                            onTap: () {
                              _controller.text = item;
                              _connect(item);
                            },
                          );
                        },
                      ),
              ),
            ],
          ),
        ),
      ),
    );
  }
}
