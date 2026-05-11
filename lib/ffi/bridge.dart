import 'dart:async';
import 'dart:ffi';
import 'dart:isolate';
import 'dart:typed_data';

import 'package:ffi/ffi.dart';

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
    _clientConnect = _library
        .lookupFunction<
          Int32 Function(Pointer<Utf8>, Uint16, Pointer<Uint8>, Uint32),
          int Function(Pointer<Utf8>, int, Pointer<Uint8>, int)
        >('rc_client_connect');
    _clientDisconnect = _library
        .lookupFunction<Int32 Function(), int Function()>(
          'rc_client_disconnect',
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
  late final int Function(Pointer<Utf8>, int, Pointer<Uint8>, int)
  _clientConnect;
  late final int Function() _clientDisconnect;
  late final int Function(int) _frameStreamStart;
  late final int Function() _frameStreamStop;
  late final int Function(double, double) _sendMouseMove;
  late final int Function(double, double, int) _sendMouseDown;
  late final int Function(double, double, int) _sendMouseUp;

  final StreamController<NativeFrame> _frames =
      StreamController<NativeFrame>.broadcast();
  ReceivePort? _framePort;

  Stream<NativeFrame> get frames => _frames.stream;

  int get abiVersion => _nativeAbiVersion();

  int initializeDartApi() =>
      _initializeDartApi(NativeApi.initializeApiDLData.cast<Void>());

  int connect(String ipAddress, {int port = 0}) {
    final address = ipAddress.toNativeUtf8();
    final psk = calloc<Uint8>(_defaultPsk.length);
    try {
      for (var index = 0; index < _defaultPsk.length; index += 1) {
        psk[index] = _defaultPsk[index];
      }
      return _clientConnect(address, port, psk, _defaultPsk.length);
    } finally {
      calloc.free(psk);
      calloc.free(address);
    }
  }

  int disconnect() {
    stopFrameStream();
    return _clientDisconnect();
  }

  int startFrameStream() {
    final initializeResult = initializeDartApi();
    if (initializeResult != 0) {
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
