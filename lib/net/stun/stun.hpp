#ifndef __VJIK_NET_STUN_HPP
#define __VJIK_NET_STUN_HPP

#include "loop.hpp"
#include <cstdint>
#include <functional>
#include <netdb.h>
#include <random>
#include <string>
#include <iostream>
#include <sys/socket.h>

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

class proto {
public:
  proto() : socket_(get_socket()), ev_loop_() {
    ev_loop_.watch_read_ops_async(socket_, std::bind(write_message_handler{socket_}));
    ev_loop_.run();
  }

private:
  class write_message_handler {
  public:
    write_message_handler(int sock_fd) : sock_fd_(sock_fd) {}
  public:
    bool operator()() {
      auto msg = message{message_type::BINDING_REQUEST};
      return true;
    }
  
  private:
    int sock_fd_;
  };

  auto get_socket () const -> int {
    const char* stun_host = "stun.l.google.com";
    const char* stun_port = "19302";

    struct addrinfo hints{};
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    struct addrinfo* res = nullptr;
    int err = getaddrinfo(stun_host, stun_port, &hints, &res);
    if (err != 0) {
        std::cerr << "getaddrinfo: " << gai_strerror(err) << "\n";
        return 1;
    }

    int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock < 0) {
        perror("socket");
        freeaddrinfo(res);
        return 1;
    }

    return sock;
  } 

private:
  int socket_;
  core::loop ev_loop_;
};

} // namespace stun
} // namespace net
} // namespace vjik

#endif // !__VJIK_NET_STUN_HPP
