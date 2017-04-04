#include <quad_thrasher.hpp>
#include <random_helper.hpp>
#include <window.hpp>

#include <args.hxx>

#include <chrono>
#include <cstdlib>
#include <thread>

#include <GL/gl.h>

namespace {
  template <typename Faker, typename BufferSwapper>
  class DrawLoop {
  public:
    DrawLoop(
      BufferSwapper swap_buffers,
      std::size_t max_texture_dimension_texels,
      std::size_t average_memory_usage_bytes,
      std::size_t delta_bytes,
      std::size_t thrash_interval_,
      bool draw_
    ) : frame_count{0}
      , thrash_interval{thrash_interval_}
      , swap_buffers{std::move(swap_buffers)}
      , generator{}
      , thrasher{
          generator,
          average_memory_usage_bytes,
          delta_bytes,
          max_texture_dimension_texels
        }
      , draw{draw_}
    {}

    bool operator()() {
      while (true) {
        glClear(GL_COLOR_BUFFER_BIT);
        if (frame_count % thrash_interval == 0) {
          thrasher.thrash(generator);
          frame_count = 0;
        }
        if (draw) {
          thrasher.draw(generator);
        }
        swap_buffers();
        ++frame_count;
      }

      return true;
    }
  private:
    std::size_t frame_count;
    std::size_t thrash_interval;
    BufferSwapper swap_buffers;
    thrasher::RandomHelper generator;
    thrasher::QuadThrasher<Faker> thrasher;
    bool draw;
  };

  template <typename Faker, typename BufferSwapper>
  DrawLoop<Faker, BufferSwapper> make_draw_loop(
    BufferSwapper swap_buffers,
    std::size_t max_texture_dimension_texels,
    std::size_t average_memory_usage_bytes,
    std::size_t delta_bytes,
    std::size_t thrash_interval,
    bool draw
  ) {
    return {
      std::move(swap_buffers),
      max_texture_dimension_texels,
      average_memory_usage_bytes,
      delta_bytes,
      thrash_interval,
      draw
    };
  }
}

int main(int argc, char **argv) {
  args::ArgumentParser arg_parser{"A texture memory thrasher"};
  args::HelpFlag help_flag{
    arg_parser, "help", "Display this message", {"help"}
  };
  args::ValueFlag<std::size_t> max_texture_flag{
    arg_parser,
    "TEXELS",
    "The maximum texture dimension in texels (largest texture is TEXELSxTEXELS). "
    "Note that if this value is more than the driver supports, the driver's "
    "maximum value will be used.",
    {'t', "texture-size"},
    100
  };
  args::ValueFlag<std::size_t> max_memory_flag{
    arg_parser,
    "BYTES",
    "The base texture memory usage cap. The actual cap is this value plus the "
    "value computed from the --delta flag",
    {'m', "memory-cap"},
    200000
  };
  args::ValueFlag<double> delta_flag{
    arg_parser,
    "PERCENT",
    "Oscillate memory usage randomly within the band [memory-cap - delta * "
    "memory-cap, memory-cap + delta * memory-cap]",
    {'d', "delta"},
    0.25
  };
  args::ValueFlag<std::size_t> interval_flag {
    arg_parser,
    "INTERVAL",
    "The number of frames between texture memory thrashes",
    {'i', "interval"},
    30
  };
  args::ValueFlag<std::size_t> width_flag{
    arg_parser, "WIDTH", "The width of a screen", {'w', "width"}, 500
  };
  args::ValueFlag<std::size_t> height_flag{
    arg_parser, "HEIGHT", "The height of a screen", {'h', "height"}, 500
  };
  args::ValueFlag<std::size_t> screen_columns_flag{
    arg_parser, "COUNT", "The number of screen columns", {'c', "columns"}, 1
  };
  args::ValueFlag<std::size_t> screen_rows_flag{
    arg_parser, "COUNT", "The number of screen rows", {'r', "rows"}, 1
  };
  args::Flag alloc_buffers_flag{
    arg_parser,
    "alloc_buffers",
    "Allocate a new source buffer for each mip upload",
    {"alloc-buffers"}
  };
  args::Flag no_draw_flag{
    arg_parser,
    "no_draw",
    "Do not draw any quads. (Textures are still created/deleted.)",
    {"no-draw"}
  };
  try {
    arg_parser.ParseCLI(argc, argv);
  } catch (args::Help) {
    printf("%s", arg_parser.Help().c_str());
    return EXIT_SUCCESS;
  } catch (args::ParseError e) {
    fprintf(stderr, "%s\n", e.what());
    printf("%s", arg_parser.Help().c_str());
    return EXIT_FAILURE;
  } catch (args::ValidationError e) {
    fprintf(stderr, "%s\n", e.what());
    printf("%s", arg_parser.Help().c_str());
    return EXIT_FAILURE;
  }

  auto delta_percent = args::get(delta_flag);
  if (delta_percent < 0. || delta_percent > 1.) {
    fprintf(stderr, "Delta percentage must be between 0 and 1\n");
    return EXIT_FAILURE;
  }

  std::size_t width = args::get(width_flag) * args::get(screen_columns_flag);
  std::size_t height = args::get(height_flag) * args::get(screen_rows_flag);

  bool result = thrasher::openWindow(
    width, height, "THEFREEZE",
    [&](auto swap_buffers) {
      glClearColor(1.0f, 0.0f, 0.0f, 1.0f);

      GLint driver_max_texture_dimension;
      glGetIntegerv(GL_MAX_TEXTURE_SIZE, &driver_max_texture_dimension);
      std::size_t max_texture_dimension = args::get(max_texture_flag);
      if (max_texture_dimension > driver_max_texture_dimension) {
        max_texture_dimension = driver_max_texture_dimension;
        fprintf(
          stderr,
          "Warning: requested texture dimension was too big, using %lu\n",
          max_texture_dimension
        );
      }

      if (alloc_buffers_flag) {
        return make_draw_loop<thrasher::UniqueBufferFaker>(
          std::move(swap_buffers),
          max_texture_dimension,
          args::get(max_memory_flag),
          args::get(max_memory_flag) * delta_percent,
          args::get(interval_flag),
          !args::get(no_draw_flag)
        )();
      } else {
        return make_draw_loop<thrasher::SharedBufferFaker>(
          std::move(swap_buffers),
          max_texture_dimension,
          args::get(max_memory_flag),
          args::get(max_memory_flag) * delta_percent,
          args::get(interval_flag),
          !args::get(no_draw_flag)
        )();
      }
    }
  );

  if (result) return EXIT_SUCCESS; else return EXIT_FAILURE;
}
