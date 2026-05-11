#include "dxgi_capture.h"

#if defined(_WIN32)

#include "capture_factory.h"

#include <algorithm>
#include <memory>

namespace rc::capture {

DxgiCapture::DxgiCapture() = default;

DxgiCapture::~DxgiCapture() {
  stop();
}

bool DxgiCapture::start() {
  if (started_) {
    return true;
  }
  started_ = initialize_device() && initialize_duplication();
  return started_;
}

FrameBuffer DxgiCapture::next_frame() {
  if (!started_ || !duplication_) {
    return {};
  }

  DXGI_OUTDUPL_FRAME_INFO frame_info{};
  Microsoft::WRL::ComPtr<IDXGIResource> resource;
  HRESULT result = duplication_->AcquireNextFrame(33, &frame_info, &resource);
  if (result == DXGI_ERROR_WAIT_TIMEOUT) {
    return {};
  }
  if (FAILED(result)) {
    stop();
    return {};
  }

  Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
  result = resource.As(&texture);
  FrameBuffer frame;
  if (SUCCEEDED(result)) {
    frame = copy_texture(texture.Get());
  }
  duplication_->ReleaseFrame();
  return frame;
}

void DxgiCapture::stop() {
  duplication_.Reset();
  context_.Reset();
  device_.Reset();
  started_ = false;
}

bool DxgiCapture::initialize_device() {
  constexpr D3D_DRIVER_TYPE driver_types[] = {
      D3D_DRIVER_TYPE_HARDWARE,
      D3D_DRIVER_TYPE_WARP,
  };
  constexpr D3D_FEATURE_LEVEL feature_levels[] = {
      D3D_FEATURE_LEVEL_11_1,
      D3D_FEATURE_LEVEL_11_0,
  };

  for (const D3D_DRIVER_TYPE driver_type : driver_types) {
    D3D_FEATURE_LEVEL selected_level{};
    const HRESULT result = D3D11CreateDevice(
        nullptr, driver_type, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
        feature_levels, static_cast<UINT>(std::size(feature_levels)),
        D3D11_SDK_VERSION, &device_, &selected_level, &context_);
    if (SUCCEEDED(result)) {
      return true;
    }
  }
  return false;
}

bool DxgiCapture::initialize_duplication() {
  Microsoft::WRL::ComPtr<IDXGIDevice> dxgi_device;
  if (FAILED(device_.As(&dxgi_device))) {
    return false;
  }

  Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
  if (FAILED(dxgi_device->GetAdapter(&adapter))) {
    return false;
  }

  Microsoft::WRL::ComPtr<IDXGIOutput> output;
  if (FAILED(adapter->EnumOutputs(0, &output))) {
    return false;
  }
  output->GetDesc(&output_desc_);

  Microsoft::WRL::ComPtr<IDXGIOutput1> output1;
  if (FAILED(output.As(&output1))) {
    return false;
  }
  return SUCCEEDED(output1->DuplicateOutput(device_.Get(), &duplication_));
}

FrameBuffer DxgiCapture::copy_texture(ID3D11Texture2D* texture) {
  if (texture == nullptr) {
    return {};
  }

  D3D11_TEXTURE2D_DESC desc{};
  texture->GetDesc(&desc);
  desc.BindFlags = 0;
  desc.MiscFlags = 0;
  desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
  desc.Usage = D3D11_USAGE_STAGING;

  Microsoft::WRL::ComPtr<ID3D11Texture2D> staging;
  if (FAILED(device_->CreateTexture2D(&desc, nullptr, &staging))) {
    return {};
  }
  context_->CopyResource(staging.Get(), texture);

  D3D11_MAPPED_SUBRESOURCE mapped{};
  if (FAILED(context_->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped))) {
    return {};
  }

  FrameBuffer frame;
  frame.width = desc.Width;
  frame.height = desc.Height;
  frame.stride = desc.Width * 4U;
  frame.format = PixelFormat::kBgra8888;
  frame.pixels.resize(static_cast<size_t>(frame.stride) * frame.height);

  const auto* source = static_cast<const uint8_t*>(mapped.pData);
  for (uint32_t row = 0; row < frame.height; ++row) {
    std::copy_n(source + static_cast<size_t>(row) * mapped.RowPitch,
                frame.stride,
                frame.pixels.data() + static_cast<size_t>(row) * frame.stride);
  }

  context_->Unmap(staging.Get(), 0);
  return frame;
}

std::unique_ptr<ICapture> create_platform_capture() {
  return std::make_unique<DxgiCapture>();
}

}  // namespace rc::capture

#endif  // defined(_WIN32)
