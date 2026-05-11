#include "android_capture.h"

#if defined(__ANDROID__)

#include "capture_factory.h"

#include <memory>

namespace rc::capture {

bool AndroidCapture::start() {
  return true;
}

FrameBuffer AndroidCapture::next_frame() {
  return {};
}

void AndroidCapture::stop() {}

std::unique_ptr<ICapture> create_platform_capture() {
  return std::make_unique<AndroidCapture>();
}

}  // namespace rc::capture

#endif  // defined(__ANDROID__)
