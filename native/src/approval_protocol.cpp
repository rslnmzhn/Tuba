#include "approval_protocol.h"

#include <cstring>
#include <vector>

namespace rc {
namespace {

constexpr char kRequestPrefix[] = "REQUEST{";
constexpr std::size_t kMaxProtocolMessage = 512;

std::string sanitize_device_name(const char* device_name) {
  if (device_name == nullptr || device_name[0] == '\0') {
    return "Unknown device";
  }
  std::string value(device_name);
  if (value.size() > 80U) {
    value.resize(80U);
  }
  for (char& ch : value) {
    if (ch == '{' || ch == '}' || ch == '\n' || ch == '\r' || ch == '\0') {
      ch = ' ';
    }
  }
  return value;
}

}  // namespace

std::string make_connection_request(const char* device_name) {
  return std::string(kRequestPrefix) + sanitize_device_name(device_name) + "}";
}

bool parse_connection_request(const std::string& message,
                              std::string* device_name) {
  if (device_name == nullptr || message.rfind(kRequestPrefix, 0) != 0 ||
      message.empty() || message.back() != '}') {
    return false;
  }
  *device_name = message.substr(std::strlen(kRequestPrefix),
                                message.size() - std::strlen(kRequestPrefix) - 1U);
  return true;
}

bool send_protocol_message(transport::TlsTransport& session,
                           const std::string& message) {
  if (message.empty() || message.size() > kMaxProtocolMessage) {
    return false;
  }
  const auto send_all = [&session](const uint8_t* data, std::size_t length) {
    std::size_t sent = 0;
    while (sent < length) {
      const int result = session.send(data + sent, length - sent);
      if (result <= 0) {
        return false;
      }
      sent += static_cast<std::size_t>(result);
    }
    return true;
  };
  const uint32_t length = static_cast<uint32_t>(message.size());
  const uint8_t header[4] = {
      static_cast<uint8_t>((length >> 24U) & 0xFFU),
      static_cast<uint8_t>((length >> 16U) & 0xFFU),
      static_cast<uint8_t>((length >> 8U) & 0xFFU),
      static_cast<uint8_t>(length & 0xFFU),
  };
  return send_all(header, sizeof(header)) &&
         send_all(reinterpret_cast<const uint8_t*>(message.data()), message.size());
}

bool recv_protocol_message(transport::TlsTransport& session,
                           std::string* message) {
  if (message == nullptr) {
    return false;
  }
  const auto recv_all = [&session](uint8_t* data, std::size_t length) {
    std::size_t received = 0;
    while (received < length) {
      const int result = session.recv(data + received, length - received);
      if (result <= 0) {
        return false;
      }
      received += static_cast<std::size_t>(result);
    }
    return true;
  };
  uint8_t header[4]{};
  if (!recv_all(header, sizeof(header))) {
    return false;
  }
  const uint32_t length = (static_cast<uint32_t>(header[0]) << 24U) |
                          (static_cast<uint32_t>(header[1]) << 16U) |
                          (static_cast<uint32_t>(header[2]) << 8U) |
                          static_cast<uint32_t>(header[3]);
  if (length == 0U || length > kMaxProtocolMessage) {
    return false;
  }
  std::vector<uint8_t> buffer(length);
  if (!recv_all(buffer.data(), buffer.size())) {
    return false;
  }
  *message = std::string(reinterpret_cast<const char*>(buffer.data()), buffer.size());
  return true;
}

}  // namespace rc
