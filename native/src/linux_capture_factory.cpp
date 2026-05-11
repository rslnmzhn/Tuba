#include "capture_factory.h"

#if defined(__linux__) && !defined(__ANDROID__)

#include "pipewire_capture.h"

#include <memory>

namespace rc::capture {

std::unique_ptr<ICapture> create_platform_capture() {
  return std::make_unique<PipeWireCapture>();
}

}  // namespace rc::capture

#endif  // defined(__linux__) && !defined(__ANDROID__)
