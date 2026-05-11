#include "h264_encoder.h"

#if defined(RC_NATIVE_ENABLE_OPENH264)

#include <libyuv/convert.h>

#include <algorithm>
#include <cstring>

namespace rc::codec {
namespace {

constexpr int kTargetBitrate = 2'000'000;

bool supported_format(capture::PixelFormat format) {
  return format == capture::PixelFormat::kBgra8888 ||
         format == capture::PixelFormat::kRgba8888;
}

}  // namespace

H264Encoder::H264Encoder() = default;

H264Encoder::~H264Encoder() {
  stop();
}

bool H264Encoder::start(uint32_t width, uint32_t height, uint32_t fps) {
  stop();
  if (width == 0U || height == 0U || fps == 0U) {
    return false;
  }

  if (WelsCreateSVCEncoder(&encoder_) != 0 || encoder_ == nullptr) {
    return false;
  }

  SEncParamBase params{};
  params.iUsageType = CAMERA_VIDEO_REAL_TIME;
  params.fMaxFrameRate = static_cast<float>(fps);
  params.iPicWidth = static_cast<int>(width);
  params.iPicHeight = static_cast<int>(height);
  params.iTargetBitrate = kTargetBitrate;
  if (encoder_->Initialize(&params) != cmResultSuccess) {
    stop();
    return false;
  }

  width_ = width;
  height_ = height;
  fps_ = fps;
  y_plane_.resize(static_cast<size_t>(width_) * height_);
  u_plane_.resize(static_cast<size_t>((width_ + 1U) / 2U) * ((height_ + 1U) / 2U));
  v_plane_.resize(u_plane_.size());
  return true;
}

EncodedPacket H264Encoder::encode(const capture::FrameBuffer& frame) {
  if (encoder_ == nullptr || !convert_to_i420(frame)) {
    return {};
  }

  SSourcePicture picture{};
  picture.iPicWidth = static_cast<int>(width_);
  picture.iPicHeight = static_cast<int>(height_);
  picture.iColorFormat = videoFormatI420;
  picture.iStride[0] = static_cast<int>(width_);
  picture.iStride[1] = static_cast<int>((width_ + 1U) / 2U);
  picture.iStride[2] = picture.iStride[1];
  picture.pData[0] = y_plane_.data();
  picture.pData[1] = u_plane_.data();
  picture.pData[2] = v_plane_.data();

  SFrameBSInfo info{};
  if (encoder_->EncodeFrame(&picture, &info) != cmResultSuccess) {
    return {};
  }
  return packet_from_info(info);
}

void H264Encoder::stop() {
  if (encoder_ != nullptr) {
    encoder_->Uninitialize();
    WelsDestroySVCEncoder(encoder_);
    encoder_ = nullptr;
  }
  width_ = 0;
  height_ = 0;
  fps_ = 0;
  y_plane_.clear();
  u_plane_.clear();
  v_plane_.clear();
}

bool H264Encoder::convert_to_i420(const capture::FrameBuffer& frame) {
  if (frame.width != width_ || frame.height != height_ ||
      frame.stride < width_ * 4U || !supported_format(frame.format)) {
    return false;
  }

  const int half_stride = static_cast<int>((width_ + 1U) / 2U);
  const int result = frame.format == capture::PixelFormat::kBgra8888
                         ? libyuv::BGRAToI420(frame.pixels.data(),
                                              static_cast<int>(frame.stride),
                                              y_plane_.data(),
                                              static_cast<int>(width_),
                                              u_plane_.data(), half_stride,
                                              v_plane_.data(), half_stride,
                                              static_cast<int>(width_),
                                              static_cast<int>(height_))
                         : libyuv::RGBAToI420(frame.pixels.data(),
                                              static_cast<int>(frame.stride),
                                              y_plane_.data(),
                                              static_cast<int>(width_),
                                              u_plane_.data(), half_stride,
                                              v_plane_.data(), half_stride,
                                              static_cast<int>(width_),
                                              static_cast<int>(height_));
  return result == 0;
}

EncodedPacket H264Encoder::packet_from_info(const SFrameBSInfo& info) const {
  if (info.eFrameType == videoFrameTypeSkip) {
    return {};
  }

  EncodedPacket packet;
  packet.codec = CodecId::kH264;
  packet.width = width_;
  packet.height = height_;
  packet.key_frame = info.eFrameType == videoFrameTypeIDR;

  for (int layer = 0; layer < info.iLayerNum; ++layer) {
    const SLayerBSInfo& layer_info = info.sLayerInfo[layer];
    int layer_size = 0;
    for (int nal = 0; nal < layer_info.iNalCount; ++nal) {
      layer_size += layer_info.pNalLengthInByte[nal];
    }
    const uint8_t* begin = layer_info.pBsBuf;
    packet.bytes.insert(packet.bytes.end(), begin, begin + layer_size);
  }
  return packet;
}

}  // namespace rc::codec

#endif  // defined(RC_NATIVE_ENABLE_OPENH264)
