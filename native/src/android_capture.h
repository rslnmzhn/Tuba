#ifndef RC_NATIVE_ANDROID_CAPTURE_H_
#define RC_NATIVE_ANDROID_CAPTURE_H_

#include "capture.h"

namespace rc::capture {

class AndroidCapture final : public ICapture {
 public:
  bool start() override;
  FrameBuffer next_frame() override;
  void stop() override;
};

}  // namespace rc::capture

#endif  // RC_NATIVE_ANDROID_CAPTURE_H_
