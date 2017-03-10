#include <quad_thrasher.hpp>
#include <random_helper.hpp>
#include <window.hpp>

#include <chrono>
#include <thread>

#include <GL/gl.h>

//constexpr std::size_t max_texture_bytes = 101782080;
//constexpr std::size_t max_requested_memory_bytes = 2000000000;
constexpr std::size_t max_texture_bytes = 10000;
constexpr std::size_t max_requested_memory_bytes = 200000;

int main() {
  // TODO command line args
  bool result = forensics::openWindow(
    //1920 * 3, 1080, "THEFREEZE",
    500, 500, "THEFREEZE",
    [](auto swap_buffers) {
      glClearColor(1.0f, 0.0f, 0.0f, 1.0f);

      forensics::RandomHelper generator{};
      forensics::QuadThrasher thrasher{generator, max_requested_memory_bytes, max_texture_bytes};

      while (true) {
        glClear(GL_COLOR_BUFFER_BIT);
        thrasher.thrash(generator);
        thrasher.draw(generator);
        swap_buffers();
        //using namespace std::chrono_literals;
        //std::this_thread::sleep_for(0.5s);
      }

      return true;
    }
  );

  if (result) return 0; else return 1;
}
