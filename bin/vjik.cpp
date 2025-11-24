#include "stun.hpp"
#include "loop.hpp"

namespace core = vjik::net::core;
namespace stun = vjik::net::stun;

int main() {
  try {
    auto loop = core::event_loop{};
    auto handler = stun::proto{loop};
    loop.run();
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
  }
}
