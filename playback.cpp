#include <random_quad.hpp>
#include <window.hpp>

#include <chrono>
#include <cmath>
#include <thread>
#include <unordered_map>
#include <GL/gl.h>

#ifndef LOG_FILE
  #define LOG_FILE "/tmp/gl_playback.cpp"
#endif

namespace {
  template <typename Swapper>
  class Playback final {
    static constexpr std::size_t max_texture_bytes = 101782080;

  public:
    Playback(Swapper const &swap_buffers_)
      : quads{}
      , swap_buffers{std::move(swap_buffers_)}
      , faker{}
    {}

    void create_quad(std::size_t id, GLsizei width, GLsizei height) {
      // Yeah, this is overkill.
      quads.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(id),
        std::forward_as_tuple(width, height, faker)
      );
    }

    void delete_quad(std::size_t id) {
      quads.erase(id);
    }

    void draw() {
      glClear(GL_COLOR_BUFFER_BIT);

      for (auto &pair : quads) {
        auto &quad = pair.second;
        quad.draw();
      }

      auto err = glGetError();
      if (err != 0) {
        fprintf(stderr, "GL ERROR: %d\n", err);
      }

      swap_buffers();
    }

  private:
    std::unordered_map<std::size_t, forensics::RandomQuad<max_texture_bytes>> quads;
    Swapper const &swap_buffers;
    forensics::BufferFaker<max_texture_bytes> faker;
  };

  template <typename Swapper>
  Playback<Swapper> make_playback(Swapper const &swapper) {
    return {swapper};
  }
}

int main() {
  // TODO command line args
  bool result = forensics::openWindow(
    1920 * 3, 1080, "THEFREEZE",
    [](auto swap_buffers) {
      glClearColor(1.0f, 0.0f, 0.0f, 1.0f);

      while (true) {
        auto playback = make_playback(swap_buffers);

#include LOG_FILE

        using namespace std::chrono_literals;
        std::this_thread::sleep_for(0.5s);
      }

      return true;
    }
  );

  if (result) return 0; else return 1;
}
