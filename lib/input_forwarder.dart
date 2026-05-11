import 'package:flutter/widgets.dart';

import 'ffi/bridge.dart';

final class InputForwarder {
  InputForwarder({RcBridge? bridge}) : _bridge = bridge ?? RcBridge.instance;

  static const int primaryButton = 1;

  final RcBridge _bridge;
  Size _viewportSize = Size.zero;

  void updateViewport(Size size) {
    _viewportSize = size;
  }

  void handleTapDown(TapDownDetails details) {
    final position = _normalize(details.localPosition);
    _bridge.sendMouseDown(position.dx, position.dy, primaryButton);
    _bridge.sendMouseUp(position.dx, position.dy, primaryButton);
  }

  void handlePanStart(DragStartDetails details) {
    final position = _normalize(details.localPosition);
    _bridge.sendMouseDown(position.dx, position.dy, primaryButton);
  }

  void handlePanUpdate(DragUpdateDetails details) {
    final position = _normalize(details.localPosition);
    _bridge.sendMouseMove(position.dx, position.dy);
  }

  void handlePanEnd(DragEndDetails details) {
    _bridge.sendMouseUp(0, 0, primaryButton);
  }

  Offset _normalize(Offset localPosition) {
    if (_viewportSize.width <= 0 || _viewportSize.height <= 0) {
      return Offset.zero;
    }
    return Offset(
      (localPosition.dx / _viewportSize.width).clamp(0.0, 1.0),
      (localPosition.dy / _viewportSize.height).clamp(0.0, 1.0),
    );
  }
}
