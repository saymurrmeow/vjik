#ifndef __VJIK_NET_CORE_LOOP_HPP
#define __VJIK_NET_CORE_LOOP_HPP

#include <algorithm>
#include <cstring>
#include <functional>
#include <netdb.h>
#include <print>
#include <sys/poll.h>
#include <time.h>
#include <unistd.h>
#include <unordered_map>
#include <utility>

namespace vjik {
namespace net {
namespace core {

class loop {
  using Handler = std::function<bool()>;

public:
  loop() {};
  loop(loop &) = delete;
  loop &operator=(loop &) = delete;
  // TODO: move constructor/assigment....
  loop(loop &&);
  loop &operator=(loop &&);
  virtual ~loop() {}

public:
  auto run() {
    while (!shutdown_) {
      int max_fd = -1;

      fd_set read_fds;
      FD_ZERO(&read_fds);
      for (const auto &it : read_queue_) {
        FD_SET(it.first, &read_fds);
        max_fd = std::max(max_fd, it.first);
      }

      timeval tv_buf = {1, 0};
      std::println("loop::run(): before select");
      int nready = ::select(max_fd + 1, &read_fds, nullptr, nullptr, &tv_buf);
      std::println("loop::run(): after select");

      if (nready > 0) {
        auto it = read_queue_.begin();
        while (it != read_queue_.end()) {
          auto fd = it->first;
          auto fn = it->second;
          ++it;
          if (FD_ISSET(fd, &read_fds)) {
            auto done = fn(); 
            if (done) { read_queue_.erase(fd); }
          }
        }
      }
    }
  }

  auto watch_read_ops_async(int sock_fd, Handler h) {
    read_queue_.insert({ sock_fd, h });
    std::println("loop::watch_read_ops_async for fd {}", sock_fd);
  }

private:
  bool shutdown_;
  std::unordered_map<int, Handler> read_queue_;
  std::unordered_map<int, Handler> write_queue_;
};

} // namespace core
} // namespace net
} // namespace vjik
#endif
