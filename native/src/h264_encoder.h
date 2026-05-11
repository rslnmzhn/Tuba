#ifndef RC_NATIVE_H264_ENCODER_H_
#define RC_NATIVE_H264_ENCODER_H_

#include "codec.h"

#if defined(RC_NATIVE_ENABLE_OPENH264)

#include <codec_api.h>

#include <cstdint>
#include <memory>
#include <vector>

namespace rc::codec {

class H264Encoder final : public IEncoder {
 public:
  H264Encoder();
  ~H264Encoder() override;

  H264Encoder(const H264Encoder&) = delete;
  H264Encoder& operator=(const H264Encoder&) = delete;

  bool start(uint32_t width, uint32_t height, uint32_t fps) override;
  EncodedPacket encode(const capture::FrameBuffer& frame) override;
  void stop() override;

 private:
  bool convert_to_i420(const capture::FrameBuffer& frame);
  EncodedPacket packet_from_info(const SFrameBSInfo& info) const;

  ISVCEncoder* encoder_ = nullptr;
  uint32_t width_ = 0;
  uint32_t height_ = 0;
  uint32_t fps_ = 0;
  std::vector<uint8_t> y_plane_;
  std::vector<uint8_t> u_plane_;
  std::vector<uint8_t> v_plane_;
};

}  // namespace rc::codec

#endif  // defined(RC_NATIVE_ENABLE_OPENH264)

#endif  // RC_NATIVE_H264_ENCODER_H_
