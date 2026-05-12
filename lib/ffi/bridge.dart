import 'dart:async';
import 'dart:ffi';
import 'dart:isolate';
import 'dart:typed_data';

import 'package:ffi/ffi.dart';

import '../debug_log.dart';
import 'native_lib.dart';

const List<int> _defaultPsk = <int>[
  0x74,
  0x75,
  0x62,
  0x61,
  0x2d,
  0x70,
  0x73,
  0x6b,
  0x2d,
  0x76,
  0x31,
];

final class NativeFrame {
  const NativeFrame({
    required this.width,
    required this.height,
    required this.rgba,
  });

  final int width;
  final int height;
  final Uint8List rgba;
}

final class DiscoveredDevice {
  const DiscoveredDevice({
    required this.name,
    required this.ipAddress,
    required this.tcpPort,
  });

  final String name;
  final String ipAddress;
  final int tcpPort;
}

final class ApprovalRequest {
  const ApprovalRequest({
    required this.id,
    required this.deviceName,
    required this.ipAddress,
  });

  final int id;
  final String deviceName;
  final String ipAddress;
}

final class RcBridge {
  RcBridge._() : _library = NativeLib.instance {
    _nativeAbiVersion = _library
        .lookupFunction<Int32 Function(), int Function()>(
          'rc_native_abi_version',
        );
    _initializeDartApi = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>),
          int Function(Pointer<Void>)
        >('rc_initialize_dart_api');
    _clientConnectNamed = _library
        .lookupFunction<
          Int32 Function(
            Pointer<Utf8>,
            Uint16,
            Pointer<Uint8>,
            Uint32,
            Pointer<Utf8>,
          ),
          int Function(Pointer<Utf8>, int, Pointer<Uint8>, int, Pointer<Utf8>)
        >('rc_client_connect_named');
    _clientDisconnect = _library
        .lookupFunction<Int32 Function(), int Function()>(
          'rc_client_disconnect',
        );
    _serverStart = _library
        .lookupFunction<
          Int32 Function(Uint16, Pointer<Uint8>, Uint32),
          int Function(int, Pointer<Uint8>, int)
        >('rc_server_start');
    _serverStartAsync = _library
        .lookupFunction<
          Int32 Function(Uint16, Pointer<Uint8>, Uint32),
          int Function(int, Pointer<Uint8>, int)
        >('rc_server_start_async');
    _serverSetApprovalPort = _library
        .lookupFunction<Int32 Function(Int64), int Function(int)>(
          'rc_server_set_approval_port',
        );
    _serverApprovePending = _library
        .lookupFunction<Int32 Function(Int32), int Function(int)>(
          'rc_server_approve_pending',
        );
    _serverRejectPending = _library
        .lookupFunction<Int32 Function(Int32), int Function(int)>(
          'rc_server_reject_pending',
        );
    _discoveryStart = _library
        .lookupFunction<
          Int32 Function(Uint16, Pointer<Utf8>),
          int Function(int, Pointer<Utf8>)
        >('rc_discovery_start');
    _discoveryQuery = _library
        .lookupFunction<
          Int32 Function(Int64, Uint16, Uint32),
          int Function(int, int, int)
        >('rc_discovery_query');
    _lastError = _library
        .lookupFunction<Pointer<Utf8> Function(), Pointer<Utf8> Function()>(
          'rc_last_error',
        );
    _frameStreamStart = _library
        .lookupFunction<Int32 Function(Int64), int Function(int)>(
          'rc_frame_stream_start',
        );
    _frameStreamStop = _library
        .lookupFunction<Int32 Function(), int Function()>(
          'rc_frame_stream_stop',
        );
    _sendMouseMove = _library
        .lookupFunction<
          Int32 Function(Double, Double),
          int Function(double, double)
        >('rc_send_mouse_move');
    _sendMouseDown = _library
        .lookupFunction<
          Int32 Function(Double, Double, Int32),
          int Function(double, double, int)
        >('rc_send_mouse_down');
    _sendMouseUp = _library
        .lookupFunction<
          Int32 Function(Double, Double, Int32),
          int Function(double, double, int)
        >('rc_send_mouse_up');
  }

  static final RcBridge instance = RcBridge._();

  final DynamicLibrary _library;
  late final int Function() _nativeAbiVersion;
  late final int Function(Pointer<Void>) _initializeDartApi;
  late final int Function(
    Pointer<Utf8>,
    int,
    Pointer<Uint8>,
    int,
    Pointer<Utf8>,
  )
  _clientConnectNamed;
  late final int Function() _clientDisconnect;
  late final int Function(int, Pointer<Uint8>, int) _serverStart;
  late final int Function(int, Pointer<Uint8>, int) _serverStartAsync;
  late final int Function(int) _serverSetApprovalPort;
  late final int Function(int) _serverApprovePending;
  late final int Function(int) _serverRejectPending;
  late final int Function(int, Pointer<Utf8>) _discoveryStart;
  late final int Function(int, int, int) _discoveryQuery;
  late final Pointer<Utf8> Function() _lastError;
  late final int Function(int) _frameStreamStart;
  late final int Function() _frameStreamStop;
  late final int Function(double, double) _sendMouseMove;
  late final int Function(double, double, int) _sendMouseDown;
  late final int Function(double, double, int) _sendMouseUp;

  final StreamController<NativeFrame> _frames =
      StreamController<NativeFrame>.broadcast();
  final StreamController<DiscoveredDevice> _devices =
      StreamController<DiscoveredDevice>.broadcast();
  final StreamController<ApprovalRequest> _approvalRequests =
      StreamController<ApprovalRequest>.broadcast();
  ReceivePort? _framePort;
  ReceivePort? _discoveryPort;
  ReceivePort? _approvalPort;

  Stream<NativeFrame> get frames => _frames.stream;
  Stream<DiscoveredDevice> get devices => _devices.stream;
  Stream<ApprovalRequest> get approvalRequests => _approvalRequests.stream;

  int get abiVersion => _nativeAbiVersion();

  int initializeDartApi() {
    final result = _initializeDartApi(
      NativeApi.initializeApiDLData.cast<Void>(),
    );
    if (result != 0) {
      DebugLog.instance.add('rc_initialize_dart_api -> $result');
    }
    return result;
  }

  int connect(String ipAddress, {int port = 0}) {
    return connectNamed(ipAddress, deviceName: 'Tuba client', port: port);
  }

  int connectNamed(
    String ipAddress, {
    required String deviceName,
    int port = 0,
  }) {
    final address = ipAddress.toNativeUtf8();
    final name = deviceName.toNativeUtf8();
    final psk = calloc<Uint8>(_defaultPsk.length);
    try {
      for (var index = 0; index < _defaultPsk.length; index += 1) {
        psk[index] = _defaultPsk[index];
      }
      return _clientConnectNamed(address, port, psk, _defaultPsk.length, name);
    } finally {
      calloc.free(psk);
      calloc.free(name);
      calloc.free(address);
    }
  }

  int startServer({int port = 0}) {
    return _startServerWith(_serverStart, port: port);
  }

  int startServerAsync({int port = 0}) {
    return _startServerWith(_serverStartAsync, port: port);
  }

  int _startServerWith(
    int Function(int, Pointer<Uint8>, int) start, {
    required int port,
  }) {
    final psk = calloc<Uint8>(_defaultPsk.length);
    try {
      for (var index = 0; index < _defaultPsk.length; index += 1) {
        psk[index] = _defaultPsk[index];
      }
      return start(port, psk, _defaultPsk.length);
    } finally {
      calloc.free(psk);
    }
  }

  int startApprovalListener() {
    final initializeResult = initializeDartApi();
    if (initializeResult != 0) {
      DebugLog.instance.add(
        'startApprovalListener aborted: Dart DL init failed -> '
        '$initializeResult',
      );
      return initializeResult;
    }
    _approvalPort?.close();
    final port = ReceivePort('rc_approval_requests');
    _approvalPort = port;
    port.listen(_handleApprovalMessage);
    return _serverSetApprovalPort(port.sendPort.nativePort);
  }

  int approveRequest(int requestId) => _serverApprovePending(requestId);

  int rejectRequest(int requestId) => _serverRejectPending(requestId);

  int startDiscoveryResponder({int port = 0, String deviceName = 'Tuba'}) {
    final name = deviceName.toNativeUtf8();
    try {
      return _discoveryStart(port, name);
    } finally {
      calloc.free(name);
    }
  }

  int queryDiscovery({int port = 0, int timeoutMs = 800}) {
    final initializeResult = initializeDartApi();
    if (initializeResult != 0) {
      DebugLog.instance.add(
        'queryDiscovery aborted: Dart DL init failed -> $initializeResult',
      );
      return initializeResult;
    }
    _discoveryPort?.close();
    final receivePort = ReceivePort('rc_discovery');
    _discoveryPort = receivePort;
    receivePort.listen(_handleDiscoveryMessage);
    return _discoveryQuery(receivePort.sendPort.nativePort, port, timeoutMs);
  }

  String lastError() {
    final message = _lastError();
    if (message == nullptr) {
      return '';
    }
    return message.toDartString();
  }

  int disconnect() {
    stopFrameStream();
    return _clientDisconnect();
  }

  int startFrameStream() {
    final initializeResult = initializeDartApi();
    if (initializeResult != 0) {
      DebugLog.instance.add(
        'startFrameStream aborted: Dart DL init failed -> $initializeResult',
      );
      return initializeResult;
    }
    _framePort?.close();
    final port = ReceivePort('rc_frame_stream');
    _framePort = port;
    port.listen(_handleFrameMessage);
    return _frameStreamStart(port.sendPort.nativePort);
  }

  void stopFrameStream() {
    _frameStreamStop();
    _framePort?.close();
    _framePort = null;
  }

  void _handleDiscoveryMessage(Object? message) {
    if (message is! List<Object?> || message.length != 4) {
      return;
    }
    if (message[0] != 'device') {
      return;
    }
    final name = message[1];
    final ipAddress = message[2];
    final tcpPort = message[3];
    if (name is! String || ipAddress is! String || tcpPort is! int) {
      return;
    }
    _devices.add(
      DiscoveredDevice(name: name, ipAddress: ipAddress, tcpPort: tcpPort),
    );
  }

  void _handleApprovalMessage(Object? message) {
    if (message is! List<Object?> || message.length != 4) {
      return;
    }
    if (message[0] != 'approval_request') {
      return;
    }
    final id = message[1];
    final deviceName = message[2];
    final ipAddress = message[3];
    if (id is! int || deviceName is! String || ipAddress is! String) {
      return;
    }
    _approvalRequests.add(
      ApprovalRequest(id: id, deviceName: deviceName, ipAddress: ipAddress),
    );
  }

  int sendMouseMove(double x, double y) => _sendMouseMove(x, y);

  int sendMouseDown(double x, double y, int button) =>
      _sendMouseDown(x, y, button);

  int sendMouseUp(double x, double y, int button) => _sendMouseUp(x, y, button);

  void _handleFrameMessage(Object? message) {
    if (message is! List<Object?> || message.length != 4) {
      return;
    }
    final width = message[0];
    final height = message[1];
    final format = message[2];
    final pixels = message[3];
    if (width is! int ||
        height is! int ||
        format is! int ||
        pixels is! Uint8List) {
      return;
    }
    final rgba = format == 1 ? _bgraToRgba(pixels) : Uint8List.fromList(pixels);
    _frames.add(NativeFrame(width: width, height: height, rgba: rgba));
  }

  Uint8List _bgraToRgba(Uint8List bgra) {
    final rgba = Uint8List(bgra.length);
    for (var index = 0; index + 3 < bgra.length; index += 4) {
      rgba[index] = bgra[index + 2];
      rgba[index + 1] = bgra[index + 1];
      rgba[index + 2] = bgra[index];
      rgba[index + 3] = bgra[index + 3];
    }
    return rgba;
  }
}
