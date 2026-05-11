#ifndef RC_NATIVE_CAPTURE_FACTORY_H_
#define RC_NATIVE_CAPTURE_FACTORY_H_

#include "capture.h"

#include <memory>

namespace rc::capture {

std::unique_ptr<ICapture> create_platform_capture();

}  // namespace rc::capture

#endif  // RC_NATIVE_CAPTURE_FACTORY_H_
