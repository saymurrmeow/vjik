#ifndef __VJIK_NET_STUN_HPP
#define __VJIK_NET_STUN_HPP

#include <arpa/inet.h>
#include <cstdint>
#include <netdb.h>
#include <print>
#include <random>
#include <string>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>

#include "loop.hpp"

namespace vjik {
namespace net {
namespace stun {

constexpr uint32_t magic_cookie = 0x2112A442;

enum class message_type : uint16_t {
  BINDING_REQUEST          = 0x0001,  
  BINDING_INDICATION       = 0x0011,
  BINDING_RESPONSE         = 0x0101,
  BINDING_FAILURE_RESPONSE = 0x0111,
};

/// RFC5389 STUN message
class message {
public:
  explicit message(message_type type) : type_(type), transacion_id_(gen_transaction_id()) {};

public:
  auto type() const noexcept -> message_type {
    return type_;
  } 

  auto transaction_id() const noexcept -> std::string {
    return transacion_id_;
  }

private:
  /*
  * TODO: the transaction ID MUST be uniformly
  * and randomly chosen from the interval 0 .. 2**96-1, and SHOULD be
  * cryptographically random. 
  */
  auto gen_transaction_id() -> std::string {
    const std::string alnum =
          "0123456789"
          "abcdefghijklmnopqrstuvwxyz"
          "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    static thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<> dist(0, alnum.size() - 1);

    std::string id(12, '\0');
    for (char& c : id)
          c = alnum[dist(rng)];

    return id;
  }

private:
  message_type type_;
  std::string transacion_id_;
};

inline void fill_stun_buffer(const message& msg, std::array<uint8_t, 20>& buf) {
    buf.fill(0);

    uint16_t type = static_cast<uint16_t>(msg.type());
    uint16_t type_n = htons(type);

    uint16_t length_n = 0;

    uint32_t cookie_n = htonl(magic_cookie);

    const std::string& tid = msg.transaction_id();
    uint8_t tid_bytes[12] = {0};

    std::memcpy(tid_bytes, tid.data(), std::min<size_t>(tid.size(), 12));
    std::memcpy(buf.data() + 0, &type_n,    sizeof(type_n));
    std::memcpy(buf.data() + 2, &length_n,  sizeof(length_n));
    std::memcpy(buf.data() + 4, &cookie_n,  sizeof(cookie_n));
    std::memcpy(buf.data() + 8, tid_bytes,  sizeof(tid_bytes));
}

class proto {
public:
  proto(core::event_loop &loop) 
    : socket_(get_socket()),
      loop_(loop) {
        loop_.watch_write_ops_async(socket_, std::bind(write_message_handler{socket_, &loop}));
      }

private:
  class write_message_handler {
  public:
    write_message_handler(int sock_fd, core::event_loop *loop) 
      : sock_fd_(sock_fd),
        loop_(loop) {}

  public:
    bool operator()() {
      int nbytes;
      auto msg = message{message_type::BINDING_REQUEST};
      std::array<uint8_t, 20> buf{};
      fill_stun_buffer(msg, buf);

      if ((nbytes = ::send(sock_fd_, buf.data(), buf.size(), 0)) == -1) {
        std::perror("send");
        std::cerr << "Error on try send binding request\n";
      }

      loop_->watch_read_ops_async(sock_fd_, std::bind(read_message_handler{sock_fd_}));

      return true;
    }
  
  private:
    int sock_fd_;
    core::event_loop *loop_;
  };

  class read_message_handler {
  public:
    read_message_handler(int sock_fd) : sock_fd_(sock_fd) {}

  public:
    bool operator()() {
      int nbytes;
      auto recv_buf = std::array<char, 2048>{};
      if ((nbytes = ::recv(sock_fd_, recv_buf.data(), recv_buf.size(), 0)) == -1) {
        std::cerr << "Error on try recv binding request" << "\n";
      }

      std::println("STUN Response {} bytes", nbytes);
      for (int i = 0; i < nbytes; ++i) {
        printf("%02X ", static_cast<unsigned char>(recv_buf[i]));
      }
      std::cout << "\n";

      return true;
    }
    
    private:
      int sock_fd_;
  };


  auto get_socket () const -> int {
    const char* stun_port = "19302";
    const auto stun_host = "stun.l.google.com";
    int sock_fd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    if((rv = getaddrinfo(stun_host, stun_port, &hints, &servinfo)) != 0) {
        std::cerr << "getaddrinfo: " << gai_strerror(rv) << "\n";
        return 1;
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
      if ((sock_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
        perror("socket");
        continue;
      }

      if (connect(sock_fd, p->ai_addr, p->ai_addrlen)) {
        ::close(sock_fd);
        perror("socket connect");
        continue;
      }

      break;
    }

    if (p == nullptr) {
      std::cerr << "failed to create stun client socket" << std::endl;
      // TODO: handle error
    }

    char ipstr[INET6_ADDRSTRLEN];
    if (p->ai_family == AF_INET) {
      struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
      inet_ntop(AF_INET, &ipv4->sin_addr, ipstr, sizeof(ipstr));
    } else {
      struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
      inet_ntop(AF_INET6, &ipv6->sin6_addr, ipstr, sizeof(ipstr));
    }
    
    freeaddrinfo(servinfo);

    std::cout << "Connected to " << ipstr << "\n";

    return sock_fd;
  } 

private:
  int socket_;
  // WARN: refactor
  core::event_loop &loop_;
};

} // namespace stun
} // namespace net
} // namespace vjik

#endif // !__VJIK_NET_STUN_HPP
