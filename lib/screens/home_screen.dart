import 'dart:async';

import 'package:flutter/material.dart';

import '../connection_history.dart';
import '../discovered_devices.dart';
import '../ffi/bridge.dart';
import 'viewer_screen.dart';

class HomeScreen extends StatefulWidget {
  const HomeScreen({
    super.key,
    this.enableNativeServices = true,
    this.discoveredDevices,
  });

  final bool enableNativeServices;
  final DiscoveredDevices? discoveredDevices;

  @override
  State<HomeScreen> createState() => _HomeScreenState();
}

class _HomeScreenState extends State<HomeScreen> {
  final TextEditingController _controller = TextEditingController();
  final ConnectionHistory _connectionHistory = ConnectionHistory.instance;
  late final DiscoveredDevices _discoveredDevices;
  RcBridge get _bridge => RcBridge.instance;
  List<String> _history = <String>[];
  List<DiscoveredDevice> _devices = <DiscoveredDevice>[];
  bool _connecting = false;
  bool _waitingForApproval = false;
  StreamSubscription<List<DiscoveredDevice>>? _deviceSubscription;
  StreamSubscription<ApprovalRequest>? _approvalSubscription;

  @override
  void initState() {
    super.initState();
    _discoveredDevices = widget.discoveredDevices ?? DiscoveredDevices.instance;
    _loadHistory();
    if (widget.enableNativeServices) {
      _startNativeServices();
    }
  }

  @override
  void dispose() {
    _deviceSubscription?.cancel();
    _approvalSubscription?.cancel();
    _controller.dispose();
    super.dispose();
  }

  void _startNativeServices() {
    _bridge.startApprovalListener();
    _bridge.startServerAsync();
    _bridge.startDiscoveryResponder(deviceName: 'Tuba');
    _deviceSubscription = _discoveredDevices.stream.listen(_handleDevices);
    _approvalSubscription = _bridge.approvalRequests.listen(
      _handleApprovalRequest,
    );
    Future<void>.delayed(Duration.zero, _refreshDevices);
  }

  void _handleDevices(List<DiscoveredDevice> devices) {
    if (!mounted) {
      return;
    }
    setState(() {
      _devices = devices;
    });
  }

  Future<void> _refreshDevices() async {
    await _discoveredDevices.refresh();
  }

  Future<void> _handleApprovalRequest(ApprovalRequest request) async {
    if (!mounted) {
      _bridge.rejectRequest(request.id);
      return;
    }
    final approved = await showDialog<bool>(
      context: context,
      barrierDismissible: false,
      builder: (context) => AlertDialog(
        title: const Text('Запрос на подключение'),
        content: Text(
          'Устройство ${request.deviceName} (${request.ipAddress}) запрашивает подключение',
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.of(context).pop(false),
            child: const Text('Отклонить'),
          ),
          FilledButton(
            onPressed: () => Navigator.of(context).pop(true),
            child: const Text('Разрешить'),
          ),
        ],
      ),
    );
    if (approved == true) {
      _bridge.approveRequest(request.id);
    } else {
      _bridge.rejectRequest(request.id);
    }
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
      _waitingForApproval = true;
    });

    final result = await Future<int>(
      () => _bridge.connectNamed(ipAddress, deviceName: 'Tuba client'),
    );
    if (!mounted) {
      return;
    }

    setState(() {
      _connecting = false;
      _waitingForApproval = false;
    });

    if (result != 0) {
      final message = result == -3
          ? 'Хост отклонил подключение'
          : 'Не удалось подключиться: код $result ${_bridge.lastError()}'
                .trim();
      ScaffoldMessenger.of(
        context,
      ).showSnackBar(SnackBar(content: Text(message)));
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
              Row(
                children: [
                  Expanded(
                    child: Text(
                      'Устройства в сети',
                      style: Theme.of(context).textTheme.titleMedium,
                    ),
                  ),
                  IconButton(
                    tooltip: 'Обновить',
                    onPressed: _connecting ? null : _refreshDevices,
                    icon: const Icon(Icons.refresh),
                  ),
                ],
              ),
              const SizedBox(height: 8),
              SizedBox(
                height: 132,
                child: _devices.isEmpty
                    ? const Card(
                        child: Center(child: Text('Устройства не найдены')),
                      )
                    : ListView.separated(
                        scrollDirection: Axis.horizontal,
                        itemCount: _devices.length,
                        separatorBuilder: (_, _) => const SizedBox(width: 8),
                        itemBuilder: (context, index) {
                          final device = _devices[index];
                          return SizedBox(
                            width: 220,
                            child: Card(
                              child: InkWell(
                                borderRadius: BorderRadius.circular(12),
                                onTap: _connecting
                                    ? null
                                    : () => _connect(device.ipAddress),
                                child: Padding(
                                  padding: const EdgeInsets.all(12),
                                  child: Column(
                                    crossAxisAlignment:
                                        CrossAxisAlignment.start,
                                    mainAxisAlignment: MainAxisAlignment.center,
                                    children: [
                                      const Icon(Icons.devices),
                                      const SizedBox(height: 8),
                                      Text(
                                        device.name.isEmpty
                                            ? 'Tuba host'
                                            : device.name,
                                        maxLines: 1,
                                        overflow: TextOverflow.ellipsis,
                                        style: Theme.of(
                                          context,
                                        ).textTheme.titleSmall,
                                      ),
                                      const SizedBox(height: 4),
                                      Text(device.ipAddress),
                                    ],
                                  ),
                                ),
                              ),
                            ),
                          );
                        },
                      ),
              ),
              const SizedBox(height: 16),
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
              if (_waitingForApproval) ...[
                const SizedBox(height: 8),
                const LinearProgressIndicator(),
                const SizedBox(height: 8),
                const Text('Ожидание одобрения...'),
              ],
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
