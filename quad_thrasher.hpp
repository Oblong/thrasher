#ifndef UUID_A7972692_0ADA_41D3_B90D_F31B297F28CB
#define UUID_A7972692_0ADA_41D3_B90D_F31B297F28CB

#include <random_quad.hpp>

#include <cmath>
#include <random>

namespace forensics {
  template <typename Faker>
  class QuadThrasher final {
    static constexpr std::size_t bytes_per_texel = 4;
  public:
    QuadThrasher(
      RandomHelper &generator,
      std::size_t average_memory_usage_bytes_,
      std::size_t delta_bytes_,
      std::size_t max_texture_dimension_texels_
    ) : average_memory_usage_bytes{average_memory_usage_bytes_}
      , delta_bytes{delta_bytes_}
      , max_texture_dimension_texels{max_texture_dimension_texels_}
      , faker{
          generator,
          max_texture_dimension_texels * max_texture_dimension_texels * bytes_per_texel
        }
      , quads{}
    {}

    void thrash(RandomHelper &generator) {
      randomly_delete_quads(generator);

      std::size_t headroom_bytes = get_headroom_bytes(generator);

      while (true) {
        std::size_t width = generator.random_size(1, max_texture_dimension_texels);
        std::size_t height = generator.random_size(1, max_texture_dimension_texels);
        // 4/3 for size with mips, add 0.5 to round up
        std::size_t pending_texture_size_bound =
          (width * height * bytes_per_texel * 4. / 3.) + 0.5;
        if (pending_texture_size_bound > headroom_bytes) break;
        quads.emplace_back(width, height, faker);
        headroom_bytes -= quads.back().size_bytes();
      }
    }

    void draw(RandomHelper &generator) const {
      for (auto const &quad : quads) {
        quad.draw(generator);
      }
    }

  private:
    void randomly_delete_quads(RandomHelper &generator) {
      auto new_end = std::remove_if(
        begin(quads), end(quads), [&](auto&) { return generator.random_bool(); }
      );
      quads.erase(new_end, end(quads));
    }

    std::size_t get_bytes_used() const {
      std::vector<std::size_t> sizes{};
      for (auto &quad : quads) { sizes.push_back(quad.size_bytes()); }
      return std::accumulate(begin(sizes), end(sizes), 0);
    }

    std::size_t get_headroom_bytes(RandomHelper &generator) const {
      std::size_t max_bytes_this_thrash = generator.random_size(
        average_memory_usage_bytes - delta_bytes,
        average_memory_usage_bytes + delta_bytes
      );
      std::size_t bytes_used = get_bytes_used();
      max_bytes_this_thrash = std::max(max_bytes_this_thrash, bytes_used);

      return max_bytes_this_thrash - bytes_used;
    }

    std::size_t frame_count;
    std::size_t average_memory_usage_bytes;
    std::size_t delta_bytes;
    std::size_t max_texture_dimension_texels;
    Faker faker;
    std::vector<RandomQuad> quads;
  };
}

#endif
