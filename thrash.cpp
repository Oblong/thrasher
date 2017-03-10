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
  args::HelpFlag help_flag{arg_parser, "help", "Display this message", {"help"}};
  args::ValueFlag<std::size_t> width_flag{arg_parser, "WIDTH", "The width of a screen", {'w', "width"}, 500};
  args::ValueFlag<std::size_t> height_flag{arg_parser, "HEIGHT", "The height of a screen", {'h', "height"}, 500};
  args::ValueFlag<std::size_t> num_screens_flag{arg_parser, "SCREEN_COUNT", "The number of screens", {'s', "screens"}, 1};
  args::Flag vertical_flag{arg_parser, "vertical", "Screens are vertically aligned (defaults to horizontal)", {'v', "vertical"}};
  args::Flag alloc_buffers_flag{arg_parser, "alloc_buffers", "Allocate a new source buffer for each mip upload", {'a', "alloc_buffers"}};
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

  std::size_t width = args::get(width_flag);
  std::size_t height = args::get(height_flag);
  if (vertical_flag) {
    height *= args::get(num_screens_flag);
  } else {
    width *= args::get(num_screens_flag);
  }

  bool result = forensics::openWindow(
    width, height, "THEFREEZE",
    [&](auto swap_buffers) {
      glClearColor(1.0f, 0.0f, 0.0f, 1.0f);

      if (alloc_buffers_flag) {
        return draw_loop<forensics::SharedBufferFaker>(std::move(swap_buffers));
      } else {
        return draw_loop<forensics::UniqueBufferFaker>(std::move(swap_buffers));
      }
    }
  );

  if (result) return 0; else return 1;
}
