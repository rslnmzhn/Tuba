import 'dart:async';
import 'dart:io';

import 'package:flutter/material.dart';

import '../connection_history.dart';
import '../debug_log.dart';
import '../discovered_devices.dart';
import '../ffi/bridge.dart';
import 'debug_log_screen.dart';
import 'local_ip_card.dart';
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
  DebugLog get _debugLog => DebugLog.instance;
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
    _debugLog.add('Starting native services');
    final dartApiResult = _bridge.initializeDartApi();
    if (dartApiResult != 0) {
      _debugLog.add(
        'Native services aborted: rc_initialize_dart_api failed -> '
        '$dartApiResult',
      );
      Future<void>.delayed(Duration.zero, () {
        if (!mounted) {
          return;
        }
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(
            content: Text(
              'Не удалось инициализировать Dart DL API: $dartApiResult. '
              'Approval/discovery недоступны.',
            ),
          ),
        );
      });
      return;
    }
    final approvalResult = _bridge.startApprovalListener();
    _debugLog.add('startApprovalListener -> $approvalResult');
    if (approvalResult != 0) {
      _debugLog.add(
        'Approval listener is unavailable. Incoming connections will reach '
        'this device but cannot show an approval dialog.',
      );
    }
    final serverResult = _bridge.startServerAsync();
    _debugLog.add('startServerAsync -> $serverResult');
    final discoveryResult = _bridge.startDiscoveryResponder(deviceName: 'Tuba');
    _debugLog.add('startDiscoveryResponder -> $discoveryResult');
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
      _debugLog.add('Native services started successfully');
      return;
    }

    final nativeError = _bridge.lastError();
    _debugLog.add(
      'Native service startup failed: ${errors.join(', ')}; '
      'lastError="$nativeError"',
    );
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
    _debugLog.add('Discovery refresh requested');
    final result = await _discoveredDevices.refresh();
    final nativeError = result < 0 ? _bridge.lastError() : '';
    _debugLog.add(
      'Discovery refresh completed -> $result device event(s)'
      '${nativeError.isEmpty ? '' : '; lastError="$nativeError"'}',
    );
  }

  Future<void> _handleApprovalRequest(ApprovalRequest request) async {
    _debugLog.add(
      'Approval request received: id=${request.id}, '
      'device="${request.deviceName}", ip=${request.ipAddress}',
    );
    if (!mounted) {
      _debugLog.add(
        'Approval request ${request.id} rejected: HomeScreen unmounted',
      );
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
      final result = _bridge.approveRequest(request.id);
      _debugLog.add('Approval request ${request.id} approved -> $result');
    } else {
      final result = _bridge.rejectRequest(request.id);
      _debugLog.add('Approval request ${request.id} rejected -> $result');
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
    _debugLog.add(
      'Loaded local IPv4 addresses: '
      '${addresses.map((address) => address.address).join(', ')}',
    );
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
      _debugLog.add(
        ipAddress.isEmpty
            ? 'Connect ignored: empty IP address'
            : 'Connect ignored: connection already in progress',
      );
      return;
    }

    _debugLog.add('Connect requested: ip=$ipAddress');

    setState(() {
      _connecting = true;
      _waitingForApproval = true;
    });

    final result = await Future<int>(
      () => _bridge.connectNamed(ipAddress, deviceName: 'Tuba client'),
    );
    final nativeError = result == 0 ? '' : _bridge.lastError();
    _debugLog.add(
      'connectNamed($ipAddress) -> $result'
      '${nativeError.isEmpty ? '' : '; lastError="$nativeError"'}',
    );
    if (!mounted) {
      return;
    }

    setState(() {
      _connecting = false;
      _waitingForApproval = false;
    });

    if (result != 0) {
      final message = _connectErrorMessage(result, nativeError);
      ScaffoldMessenger.of(
        context,
      ).showSnackBar(SnackBar(content: Text(message)));
      return;
    }

    await _saveHistory(ipAddress);
    _debugLog.add('Connection succeeded: ip=$ipAddress');

    if (!mounted) {
      return;
    }
    await Navigator.of(context).push(
      MaterialPageRoute<void>(
        builder: (context) => ViewerScreen(ipAddress: ipAddress),
      ),
    );
  }

  void _openDebugLog() {
    Navigator.of(context).push(
      MaterialPageRoute<void>(builder: (context) => const DebugLogScreen()),
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
      appBar: AppBar(
        title: const Text('Tuba'),
        actions: [
          IconButton(
            tooltip: 'Debug log',
            onPressed: _openDebugLog,
            icon: const Icon(Icons.bug_report_outlined),
          ),
        ],
      ),
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
              LocalIpCard(
                addresses: _localAddresses,
                onRefresh: _loadLocalAddresses,
                onSelected: (address) => _controller.text = address,
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
