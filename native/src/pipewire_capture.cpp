#include "pipewire_capture.h"

#if defined(__linux__) && !defined(__ANDROID__)

namespace rc::capture {

bool PipeWireCapture::start() {
  return true;
}

FrameBuffer PipeWireCapture::next_frame() {
  return {};
}

void PipeWireCapture::stop() {}

}  // namespace rc::capture

#endif  // defined(__linux__) && !defined(__ANDROID__)
