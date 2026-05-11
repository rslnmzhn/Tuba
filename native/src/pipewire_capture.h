#ifndef RC_NATIVE_PIPEWIRE_CAPTURE_H_
#define RC_NATIVE_PIPEWIRE_CAPTURE_H_

#include "capture.h"

namespace rc::capture {

class PipeWireCapture final : public ICapture {
 public:
  bool start() override;
  FrameBuffer next_frame() override;
  void stop() override;
};

}  // namespace rc::capture

#endif  // RC_NATIVE_PIPEWIRE_CAPTURE_H_
