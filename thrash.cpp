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
      glClearColor(1.0f, 0.0f, 0.0f, 1.0f);

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

  struct ParsedArgs {
    std::size_t width;
    std::size_t height;
    std::size_t max_texture_dimension;
    std::size_t memory_cap;
    std::size_t delta;
    std::size_t interval;
    bool should_alloc_buffers;
    bool should_draw;

    void print() const {
      printf("width: %lu\n", width);
      printf("height: %lu\n", height);
      printf("max texture size: %lux%lu\n", max_texture_dimension, max_texture_dimension);
      printf("memory cap: %lu bytes\n", memory_cap);
      printf("delta: %lu bytes\n", delta);
      printf("interval: %lu frames\n", interval);
      printf("should alloc buffers: %s\n", should_alloc_buffers ? "true" : "false");
      printf("should draw: %s\n", should_draw ? "true" : "false");
    }
  };

  template <typename Callback>
  bool parse_args(int argc, char **argv, Callback callback) {
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
      return true;
    } catch (args::ParseError e) {
      fprintf(stderr, "%s\n", e.what());
      printf("%s", arg_parser.Help().c_str());
      return false;
    } catch (args::ValidationError e) {
      fprintf(stderr, "%s\n", e.what());
      printf("%s", arg_parser.Help().c_str());
      return false;
    }

    auto delta_percent = args::get(delta_flag);
    if (delta_percent < 0. || delta_percent > 1.) {
      fprintf(stderr, "Delta percentage must be between 0 and 1\n");
      return false;
    }

    ParsedArgs parsed;
    parsed.width = args::get(width_flag) * args::get(screen_columns_flag);
    parsed.height = args::get(height_flag) * args::get(screen_rows_flag);
    parsed.max_texture_dimension = args::get(max_texture_flag);
    parsed.memory_cap = args::get(max_memory_flag);
    parsed.delta = args::get(max_memory_flag) * delta_percent;
    parsed.interval = args::get(interval_flag);
    parsed.should_alloc_buffers = alloc_buffers_flag;
    parsed.should_draw = !args::get(no_draw_flag);

    return callback(parsed);
  }
}

int main(int argc, char **argv) {
  bool result = parse_args(argc, argv,
    [](auto &parsed) {
      return thrasher::openWindow(
        parsed.width, parsed.height, "THEFREEZE",
        [&parsed](auto swap_buffers) {
          GLint driver_max_texture_dimension;
          glGetIntegerv(GL_MAX_TEXTURE_SIZE, &driver_max_texture_dimension);
          if (parsed.max_texture_dimension > driver_max_texture_dimension) {
            parsed.max_texture_dimension = driver_max_texture_dimension;
            fprintf(
              stderr,
              "Warning: requested texture dimension was too big for driver\n"
            );
          }
          parsed.print();

          if (parsed.should_alloc_buffers) {
            return make_draw_loop<thrasher::UniqueBufferFaker>(
              std::move(swap_buffers),
              parsed.max_texture_dimension,
              parsed.memory_cap,
              parsed.delta,
              parsed.interval,
              parsed.should_draw
            )();
          } else {
            return make_draw_loop<thrasher::SharedBufferFaker>(
              std::move(swap_buffers),
              parsed.max_texture_dimension,
              parsed.memory_cap,
              parsed.delta,
              parsed.interval,
              parsed.should_draw
            )();
          }
        }
      );
    }
  );

  if (result) return EXIT_SUCCESS; else return EXIT_FAILURE;
}
