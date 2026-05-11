#ifndef RC_NATIVE_NAT_TRANSPORT_H_
#define RC_NATIVE_NAT_TRANSPORT_H_

#include "transport.h"

#include <stdexcept>

namespace rc::transport {

class NatTransport final : public ITransport {
 public:
  bool connect(const char*, uint16_t) override {
    throw std::logic_error("NatTransport is not implemented");
  }

  int send(const uint8_t*, std::size_t) override {
    throw std::logic_error("NatTransport is not implemented");
  }

  int recv(uint8_t*, std::size_t) override {
    throw std::logic_error("NatTransport is not implemented");
  }

  void disconnect() override {}
};

}  // namespace rc::transport

#endif  // RC_NATIVE_NAT_TRANSPORT_H_
