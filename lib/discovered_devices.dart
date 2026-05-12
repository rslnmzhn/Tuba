import 'dart:async';

import 'ffi/bridge.dart';

final class DiscoveredDevices {
  DiscoveredDevices._() {
    RcBridge.instance.devices.listen(_remember);
  }

  DiscoveredDevices.empty();

  static final DiscoveredDevices instance = DiscoveredDevices._();

  final Map<String, DiscoveredDevice> _devices = <String, DiscoveredDevice>{};
  final StreamController<List<DiscoveredDevice>> _controller =
      StreamController<List<DiscoveredDevice>>.broadcast();

  Stream<List<DiscoveredDevice>> get stream => _controller.stream;

  List<DiscoveredDevice> get current =>
      List<DiscoveredDevice>.unmodifiable(_devices.values);

  Future<int> refresh() async => Future<int>(RcBridge.instance.queryDiscovery);

  void _remember(DiscoveredDevice device) {
    if (device.ipAddress.isEmpty) {
      return;
    }
    _devices[device.ipAddress] = device;
    _controller.add(current);
  }
}
