import 'dart:async';
import 'dart:ui' as ui;

import 'package:flutter/material.dart';

import '../ffi/bridge.dart';
import '../input_forwarder.dart';

class ViewerScreen extends StatefulWidget {
  const ViewerScreen({super.key, required this.ipAddress});

  final String ipAddress;

  @override
  State<ViewerScreen> createState() => _ViewerScreenState();
}

class _ViewerScreenState extends State<ViewerScreen> {
  final RcBridge _bridge = RcBridge.instance;
  final InputForwarder _inputForwarder = InputForwarder();
  StreamSubscription<NativeFrame>? _subscription;
  ui.Image? _image;

  @override
  void initState() {
    super.initState();
    _subscription = _bridge.frames.listen(_updateFrame);
    final result = _bridge.startFrameStream();
    if (result != 0) {
      WidgetsBinding.instance.addPostFrameCallback((_) {
        if (!mounted) {
          return;
        }
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('Не удалось запустить трансляцию: $result')),
        );
      });
    }
  }

  @override
  void dispose() {
    _subscription?.cancel();
    _image?.dispose();
    super.dispose();
  }

  Future<void> _updateFrame(NativeFrame frame) async {
    final buffer = await ui.ImmutableBuffer.fromUint8List(frame.rgba);
    final descriptor = ui.ImageDescriptor.raw(
      buffer,
      width: frame.width,
      height: frame.height,
      pixelFormat: ui.PixelFormat.rgba8888,
    );
    final codec = await descriptor.instantiateCodec();
    final nextFrame = await codec.getNextFrame();
    descriptor.dispose();
    buffer.dispose();
    codec.dispose();
    if (!mounted) {
      nextFrame.image.dispose();
      return;
    }
    setState(() {
      _image?.dispose();
      _image = nextFrame.image;
    });
  }

  void _disconnect() {
    _bridge.disconnect();
    if (mounted) {
      Navigator.of(context).pop();
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text(widget.ipAddress),
        actions: [
          TextButton.icon(
            onPressed: _disconnect,
            icon: const Icon(Icons.close),
            label: const Text('Завершить'),
          ),
        ],
      ),
      body: PopScope(
        onPopInvokedWithResult: (_, _) => _bridge.disconnect(),
        child: LayoutBuilder(
          builder: (context, constraints) {
            final size = Size(constraints.maxWidth, constraints.maxHeight);
            _inputForwarder.updateViewport(size);
            return GestureDetector(
              behavior: HitTestBehavior.opaque,
              onTapDown: _inputForwarder.handleTapDown,
              onPanStart: _inputForwarder.handlePanStart,
              onPanUpdate: _inputForwarder.handlePanUpdate,
              onPanEnd: _inputForwarder.handlePanEnd,
              child: ColoredBox(
                color: Colors.black,
                child: Center(
                  child: _image == null
                      ? const Text(
                          'Ожидание кадров...',
                          style: TextStyle(color: Colors.white70),
                        )
                      : RawImage(
                          image: _image,
                          fit: BoxFit.contain,
                          filterQuality: FilterQuality.none,
                        ),
                ),
              ),
            );
          },
        ),
      ),
    );
  }
}
