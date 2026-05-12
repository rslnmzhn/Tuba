import 'dart:async';
import 'dart:io';

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
  List<InternetAddress> _localAddresses = <InternetAddress>[];
  bool _connecting = false;
  bool _waitingForApproval = false;
  StreamSubscription<ApprovalRequest>? _approvalSubscription;

  @override
  void initState() {
    super.initState();
    _discoveredDevices = widget.discoveredDevices ?? DiscoveredDevices.instance;
    _loadHistory();
    _loadLocalAddresses();
    if (widget.enableNativeServices) {
      _startNativeServices();
    }
  }

  @override
  void dispose() {
    _approvalSubscription?.cancel();
    _controller.dispose();
    super.dispose();
  }

  void _startNativeServices() {
    final approvalResult = _bridge.startApprovalListener();
    final serverResult = _bridge.startServerAsync();
    final discoveryResult = _bridge.startDiscoveryResponder(deviceName: 'Tuba');
    _approvalSubscription = _bridge.approvalRequests.listen(
      _handleApprovalRequest,
    );
    Future<void>.delayed(Duration.zero, _refreshDevices);
    Future<void>.delayed(Duration.zero, () {
      if (!mounted) {
        return;
      }
      _showNativeStartupDiagnostics(
        approvalResult: approvalResult,
        serverResult: serverResult,
        discoveryResult: discoveryResult,
      );
    });
  }

  void _showNativeStartupDiagnostics({
    required int approvalResult,
    required int serverResult,
    required int discoveryResult,
  }) {
    final errors = <String>[];
    if (approvalResult != 0) {
      errors.add('approval listener: $approvalResult');
    }
    if (serverResult != 0) {
      errors.add('TCP server: $serverResult');
    }
    if (discoveryResult != 0) {
      errors.add('discovery: $discoveryResult');
    }
    if (errors.isEmpty) {
      return;
    }

    final nativeError = _bridge.lastError();
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(
        content: Text(
          'Не удалось запустить сетевые службы (${errors.join(', ')}). '
          '${_permissionHint(nativeError)}',
        ),
      ),
    );
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

  Future<void> _loadLocalAddresses() async {
    final interfaces = await NetworkInterface.list(
      type: InternetAddressType.IPv4,
      includeLoopback: false,
    );
    final addresses = interfaces
        .expand((interface) => interface.addresses)
        .where((address) => !_isLinkLocalAddress(address.address))
        .toList(growable: false);
    if (!mounted) {
      return;
    }
    setState(() {
      _localAddresses = addresses;
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
      final message = _connectErrorMessage(result, _bridge.lastError());
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

  String _connectErrorMessage(int result, String nativeError) {
    final details = nativeError.trim();
    if (result == -3) {
      return 'Хост отклонил подключение. Если на хосте не появился запрос, '
          'проверьте, что Tuba запущена на хосте, порт 5900 не заблокирован '
          'фаерволом/VPN и сетевые службы стартовали без ошибок.';
    }
    if (result == -7) {
      return 'Хост доступен, но окно одобрения на нём недоступно. '
          'Перезапустите Tuba на хосте; если Windows блокирует сеть, '
          'разрешите приложение в фаерволе или запустите от имени администратора.';
    }
    final suffix = details.isEmpty ? '' : ': $details';
    return 'Не удалось подключиться (код $result)$suffix. '
        '${_permissionHint(details)}';
  }

  String _permissionHint(String nativeError) {
    final lower = nativeError.toLowerCase();
    final permissionRelated =
        lower.contains('access denied') ||
        lower.contains('permission') ||
        lower.contains('отказано') ||
        lower.contains('denied');
    if (permissionRelated) {
      return 'Похоже на запрет ОС/фаервола/VPN. Попробуйте разрешить Tuba в '
          'фаерволе или запустить приложение от имени администратора.';
    }
    return 'Если Windows/фаервол блокирует порт, разрешите Tuba в сети или '
        'запустите приложение от имени администратора.';
  }

  bool _isLinkLocalAddress(String address) => address.startsWith('169.254.');

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
              StreamBuilder<List<DiscoveredDevice>>(
                stream: _discoveredDevices.stream,
                initialData: _discoveredDevices.current,
                builder: (context, snapshot) {
                  final devices = snapshot.data ?? const <DiscoveredDevice>[];
                  return SizedBox(
                    height: 132,
                    child: devices.isEmpty
                        ? const Card(
                            child: Center(child: Text('Устройства не найдены')),
                          )
                        : ListView.separated(
                            scrollDirection: Axis.horizontal,
                            itemCount: devices.length,
                            separatorBuilder: (_, _) =>
                                const SizedBox(width: 8),
                            itemBuilder: (context, index) {
                              final device = devices[index];
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
                                        mainAxisAlignment:
                                            MainAxisAlignment.center,
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
                  );
                },
              ),
              const SizedBox(height: 16),
              Card(
                child: Padding(
                  padding: const EdgeInsets.all(12),
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Row(
                        children: [
                          Expanded(
                            child: Text(
                              'IP этого устройства',
                              style: Theme.of(context).textTheme.titleSmall,
                            ),
                          ),
                          IconButton(
                            tooltip: 'Обновить IP',
                            onPressed: _loadLocalAddresses,
                            icon: const Icon(Icons.refresh),
                          ),
                        ],
                      ),
                      const SizedBox(height: 4),
                      if (_localAddresses.isEmpty)
                        const Text('IPv4 адреса не найдены')
                      else
                        Wrap(
                          spacing: 8,
                          runSpacing: 8,
                          children: [
                            for (final address in _localAddresses)
                              InputChip(
                                avatar: const Icon(Icons.content_copy),
                                label: Text(address.address),
                                onPressed: () {
                                  _controller.text = address.address;
                                },
                              ),
                          ],
                        ),
                    ],
                  ),
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
