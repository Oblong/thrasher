#include <quad_thrasher.hpp>
#include <random_helper.hpp>
#include <window.hpp>

#include <args.hxx>

#include <chrono>
#include <thread>

#include <GL/gl.h>

namespace {
  //constexpr std::size_t max_texture_bytes = 101782080;
  //constexpr std::size_t max_requested_memory_bytes = 2000000000;
  constexpr std::size_t max_texture_bytes = 10000;
  constexpr std::size_t max_requested_memory_bytes = 200000;

  template <typename Faker, typename BufferSwapper>
  bool draw_loop(BufferSwapper swap_buffers) {
    forensics::RandomHelper generator{};
    forensics::QuadThrasher<Faker> thrasher{
      generator, max_requested_memory_bytes, max_texture_bytes
    };
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
}

int main(int argc, char **argv) {
  args::ArgumentParser arg_parser{"A texture memory thrasher"};
  args::HelpFlag help{arg_parser, "help", "Display this message", {'h', "help"}};
  args::Flag alloc_buffers{arg_parser, "alloc_buffers", "Allocate a new source buffer for each mip upload", {'a', "alloc_buffers"}};
  try {
    arg_parser.ParseCLI(argc, argv);
  } catch (args::Help) {
    printf("%s", arg_parser.Help().c_str());
    return 0;
  } catch (args::ParseError e) {
    fprintf(stderr, "%s\n", e.what());
    printf("%s", arg_parser.Help().c_str());
    return 1;
  } catch (args::ValidationError e) {
    fprintf(stderr, "%s\n", e.what());
    printf("%s", arg_parser.Help().c_str());
    return 1;
  }

  bool result = forensics::openWindow(
    //1920 * 3, 1080, "THEFREEZE",
    500, 500, "THEFREEZE",
    [&](auto swap_buffers) {
      glClearColor(1.0f, 0.0f, 0.0f, 1.0f);

      if (alloc_buffers) {
        return draw_loop<forensics::SharedBufferFaker>(std::move(swap_buffers));
      } else {
        return draw_loop<forensics::UniqueBufferFaker>(std::move(swap_buffers));
      }
    }
  );

  if (result) return 0; else return 1;
}
