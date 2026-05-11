#ifndef RC_NATIVE_DXGI_CAPTURE_H_
#define RC_NATIVE_DXGI_CAPTURE_H_

#include "capture.h"

#if defined(_WIN32)

#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

namespace rc::capture {

class DxgiCapture final : public ICapture {
 public:
  DxgiCapture();
  ~DxgiCapture() override;

  DxgiCapture(const DxgiCapture&) = delete;
  DxgiCapture& operator=(const DxgiCapture&) = delete;

  bool start() override;
  FrameBuffer next_frame() override;
  void stop() override;

 private:
  bool initialize_device();
  bool initialize_duplication();
  FrameBuffer copy_texture(ID3D11Texture2D* texture);

  Microsoft::WRL::ComPtr<ID3D11Device> device_;
  Microsoft::WRL::ComPtr<ID3D11DeviceContext> context_;
  Microsoft::WRL::ComPtr<IDXGIOutputDuplication> duplication_;
  DXGI_OUTPUT_DESC output_desc_{};
  bool started_ = false;
};

}  // namespace rc::capture

#endif  // defined(_WIN32)

#endif  // RC_NATIVE_DXGI_CAPTURE_H_
