#include "udp_discovery.h"

#include "socket_platform.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#endif

namespace rc::transport {
namespace {

constexpr char kDiscoveryProbe[] = "TUBA_DISCOVER_V1";

std::string source_ip(const sockaddr_storage& source) {
  char text[INET6_ADDRSTRLEN]{};
  if (source.ss_family == AF_INET) {
    const auto* address = reinterpret_cast<const sockaddr_in*>(&source);
    inet_ntop(AF_INET, &address->sin_addr, text, sizeof(text));
  } else if (source.ss_family == AF_INET6) {
    const auto* address = reinterpret_cast<const sockaddr_in6*>(&source);
    inet_ntop(AF_INET6, &address->sin6_addr, text, sizeof(text));
  }
  return text;
}

std::string value_after(const std::string& payload, const char* key) {
  const std::string marker = std::string(key) + "=";
  const std::size_t start = payload.find(marker);
  if (start == std::string::npos) {
    return {};
  }
  const std::size_t value_start = start + marker.size();
  const std::size_t value_end = payload.find(' ', value_start);
  return payload.substr(value_start, value_end == std::string::npos
                                         ? std::string::npos
                                         : value_end - value_start);
}

uint16_t parse_tcp_port(const std::string& payload) {
  const std::string value = value_after(payload, "tcp");
  if (value.empty()) {
    return 0;
  }
  const unsigned long parsed = std::strtoul(value.c_str(), nullptr, 10);
  return parsed <= 65535UL ? static_cast<uint16_t>(parsed) : 0;
}

}  // namespace

bool UdpDiscovery::announce_once(uint16_t port, const char* device_name,
                                 uint16_t tcp_port) {
  if (device_name == nullptr || !initialize_sockets()) {
    return false;
  }
  rc_socket_t socket_handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (socket_handle == kInvalidSocket) {
    return false;
  }

  set_broadcast(socket_handle);

  char payload[256]{};
  std::snprintf(payload, sizeof(payload), "TUBA_DEVICE_V1 name=%s tcp=%u",
                device_name, static_cast<unsigned>(tcp_port));

  sockaddr_in target{};
  target.sin_family = AF_INET;
  target.sin_port = htons(port);
  target.sin_addr.s_addr = htonl(INADDR_BROADCAST);

  const int sent = sendto(socket_handle, payload, static_cast<int>(std::strlen(payload)),
                          0, reinterpret_cast<sockaddr*>(&target), sizeof(target));
  close_socket(socket_handle);
  return sent > 0;
}

bool UdpDiscovery::query_once(uint16_t port, uint32_t timeout_ms) {
  return !query(port, timeout_ms).empty();
}

std::vector<DiscoveredDevice> UdpDiscovery::query(uint16_t port,
                                                  uint32_t timeout_ms) {
  std::vector<DiscoveredDevice> devices;
  if (!initialize_sockets()) {
    return devices;
  }
  rc_socket_t socket_handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (socket_handle == kInvalidSocket) {
    return devices;
  }

  set_broadcast(socket_handle);
  set_recv_timeout(socket_handle, timeout_ms);

  sockaddr_in target{};
  target.sin_family = AF_INET;
  target.sin_port = htons(port);
  target.sin_addr.s_addr = htonl(INADDR_BROADCAST);

  const int sent = sendto(socket_handle, kDiscoveryProbe,
                          static_cast<int>(std::strlen(kDiscoveryProbe)), 0,
                          reinterpret_cast<sockaddr*>(&target), sizeof(target));
  if (sent <= 0) {
    close_socket(socket_handle);
    return devices;
  }

  while (true) {
    char buffer[256]{};
    sockaddr_storage source{};
    socklen_t source_length = sizeof(source);
    const int received = recvfrom(socket_handle, buffer, sizeof(buffer) - 1U, 0,
                                  reinterpret_cast<sockaddr*>(&source),
                                  &source_length);
    if (received <= 0) {
      break;
    }

    const std::string payload(buffer, static_cast<std::size_t>(received));
    if (payload.rfind("TUBA_DEVICE_V1", 0) != 0) {
      continue;
    }

    DiscoveredDevice device;
    device.name = value_after(payload, "name");
    device.ip_address = source_ip(source);
    device.tcp_port = parse_tcp_port(payload);
    if (!device.ip_address.empty()) {
      devices.push_back(std::move(device));
    }
  }

  close_socket(socket_handle);
  return devices;
}

}  // namespace rc::transport
