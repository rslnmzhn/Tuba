#include "udp_discovery_responder.h"

#include "socket_platform.h"

#include <cstdio>
#include <cstring>

#if !defined(_WIN32)
#include <arpa/inet.h>
#endif

namespace rc::transport {
namespace {

constexpr char kDiscoveryProbe[] = "TUBA_DISCOVER_V1";
constexpr uint32_t kResponderPollTimeoutMs = 200;

std::string local_ip_for_peer(const sockaddr_in& peer) {
  rc_socket_t probe = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (probe == kInvalidSocket) {
    return "0.0.0.0";
  }

  std::string result = "0.0.0.0";
  if (connect(probe, reinterpret_cast<const sockaddr*>(&peer), sizeof(peer)) == 0) {
    sockaddr_in local{};
    socklen_t local_length = sizeof(local);
    if (getsockname(probe, reinterpret_cast<sockaddr*>(&local), &local_length) == 0) {
      char address[INET_ADDRSTRLEN]{};
      if (inet_ntop(AF_INET, &local.sin_addr, address, sizeof(address)) != nullptr) {
        result = address;
      }
    }
  }
  close_socket(probe);
  return result;
}

}  // namespace

UdpDiscoveryResponder::UdpDiscoveryResponder() : running_(false) {}

UdpDiscoveryResponder::~UdpDiscoveryResponder() {
  stop();
}

bool UdpDiscoveryResponder::start(uint16_t port, const char* device_name,
                                  uint16_t tcp_port) {
  if (device_name == nullptr || !initialize_sockets()) {
    return false;
  }

  bool expected = false;
  if (!running_.compare_exchange_strong(expected, true)) {
    return true;
  }

  try {
    thread_ = std::thread(&UdpDiscoveryResponder::run, this, port,
                          std::string(device_name), tcp_port);
    return true;
  } catch (...) {
    running_ = false;
    return false;
  }
}

void UdpDiscoveryResponder::stop() {
  running_ = false;
  if (thread_.joinable()) {
    thread_.join();
  }
}

void UdpDiscoveryResponder::run(uint16_t port, std::string device_name,
                                uint16_t tcp_port) {
  rc_socket_t socket_handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (socket_handle == kInvalidSocket) {
    running_ = false;
    return;
  }

  set_reuse_addr(socket_handle);
  set_recv_timeout(socket_handle, kResponderPollTimeoutMs);

  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(INADDR_ANY);
  address.sin_port = htons(port);

  if (bind(socket_handle, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0) {
    close_socket(socket_handle);
    running_ = false;
    return;
  }

  while (running_) {
    char buffer[128]{};
    sockaddr_in source{};
    socklen_t source_length = sizeof(source);
    const int received = recvfrom(socket_handle, buffer, sizeof(buffer) - 1U, 0,
                                  reinterpret_cast<sockaddr*>(&source),
                                  &source_length);
    if (received <= 0 || std::strcmp(buffer, kDiscoveryProbe) != 0) {
      continue;
    }

    const std::string ip = local_ip_for_peer(source);
    char response[256]{};
    std::snprintf(response, sizeof(response), "TUBA_DEVICE_V1 name=%s ip=%s tcp=%u",
                  device_name.c_str(), ip.c_str(), static_cast<unsigned>(tcp_port));
    sendto(socket_handle, response, static_cast<int>(std::strlen(response)), 0,
           reinterpret_cast<sockaddr*>(&source), source_length);
  }

  close_socket(socket_handle);
}

}  // namespace rc::transport
