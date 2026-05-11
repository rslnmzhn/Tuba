#ifndef RC_NATIVE_FRAME_BUFFER_H_
#define RC_NATIVE_FRAME_BUFFER_H_

#include <cstdint>
#include <vector>

namespace rc::capture {

enum class PixelFormat : uint32_t {
  kUnknown = 0,
  kBgra8888 = 1,
  kRgba8888 = 2,
};

struct FrameBuffer final {
  uint32_t width = 0;
  uint32_t height = 0;
  uint32_t stride = 0;
  PixelFormat format = PixelFormat::kUnknown;
  std::vector<uint8_t> pixels;

  bool empty() const { return pixels.empty() || width == 0U || height == 0U; }
};

}  // namespace rc::capture

#endif  // RC_NATIVE_FRAME_BUFFER_H_
