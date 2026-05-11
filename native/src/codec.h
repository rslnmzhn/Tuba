#ifndef RC_NATIVE_CODEC_H_
#define RC_NATIVE_CODEC_H_

#include "frame_buffer.h"

#include <cstdint>
#include <vector>

namespace rc::codec {

enum class CodecId : uint32_t {
  kUnknown = 0,
  kH264 = 1,
};

struct EncodedPacket final {
  CodecId codec = CodecId::kUnknown;
  uint32_t width = 0;
  uint32_t height = 0;
  bool key_frame = false;
  std::vector<uint8_t> bytes;
};

class IEncoder {
 public:
  virtual ~IEncoder() = default;

  virtual bool start(uint32_t width, uint32_t height, uint32_t fps) = 0;
  virtual EncodedPacket encode(const capture::FrameBuffer& frame) = 0;
  virtual void stop() = 0;
};

class IDecoder {
 public:
  virtual ~IDecoder() = default;

  virtual bool start() = 0;
  virtual capture::FrameBuffer decode(const EncodedPacket& packet) = 0;
  virtual void stop() = 0;
};

}  // namespace rc::codec

#endif  // RC_NATIVE_CODEC_H_
