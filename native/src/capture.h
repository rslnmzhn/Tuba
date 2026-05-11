#ifndef RC_NATIVE_CAPTURE_H_
#define RC_NATIVE_CAPTURE_H_

#include "frame_buffer.h"

namespace rc::capture {

class ICapture {
 public:
  virtual ~ICapture() = default;

  virtual bool start() = 0;
  virtual FrameBuffer next_frame() = 0;
  virtual void stop() = 0;
};

}  // namespace rc::capture

#endif  // RC_NATIVE_CAPTURE_H_
