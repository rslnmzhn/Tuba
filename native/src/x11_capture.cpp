#include "x11_capture.h"

#if defined(__linux__) && !defined(__ANDROID__)

namespace rc::capture {

bool X11Capture::start() {
  return true;
}

FrameBuffer X11Capture::next_frame() {
  return {};
}

void X11Capture::stop() {}

}  // namespace rc::capture

#endif  // defined(__linux__) && !defined(__ANDROID__)
