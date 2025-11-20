#include "stun.hpp"
#include "loop.hpp"

namespace stun = vjik::net::stun;

int main() {
  try {
    auto loop = vjik::net::core::loop{};
    auto handler = stun::proto{loop};
    loop.run();
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
  }
}
