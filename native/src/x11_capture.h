#ifndef RC_NATIVE_X11_CAPTURE_H_
#define RC_NATIVE_X11_CAPTURE_H_

#include "capture.h"

namespace rc::capture {

class X11Capture final : public ICapture {
 public:
  bool start() override;
  FrameBuffer next_frame() override;
  void stop() override;
};

}  // namespace rc::capture

#endif  // RC_NATIVE_X11_CAPTURE_H_
