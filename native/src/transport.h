#ifndef RC_NATIVE_TRANSPORT_H_
#define RC_NATIVE_TRANSPORT_H_

#include <cstddef>
#include <cstdint>

namespace rc::transport {

class ITransport {
 public:
  virtual ~ITransport() = default;

  virtual bool connect(const char* host, uint16_t port) = 0;
  virtual int send(const uint8_t* data, std::size_t length) = 0;
  virtual int recv(uint8_t* buffer, std::size_t length) = 0;
  virtual void disconnect() = 0;
};

}  // namespace rc::transport

#endif  // RC_NATIVE_TRANSPORT_H_
