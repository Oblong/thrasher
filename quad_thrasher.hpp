#ifndef UUID_A7972692_0ADA_41D3_B90D_F31B297F28CB
#define UUID_A7972692_0ADA_41D3_B90D_F31B297F28CB

#include <random_quad.hpp>

#include <random>

namespace forensics {
  template <typename Faker>
  class QuadThrasher final {
    static std::size_t level_zero_bytes(std::size_t texture_bytes) {
      return 3 * (texture_bytes / 4);
    }
  public:
    QuadThrasher(
      RandomHelper &generator,
      std::size_t max_requested_memory_bytes_,
      std::size_t delta_bytes_,
      std::size_t max_texture_bytes_
    ) : max_requested_memory_bytes{max_requested_memory_bytes_}
      , delta_bytes{delta_bytes_}
      , max_texture_bytes{max_texture_bytes_}
      , faker{generator, level_zero_bytes(max_texture_bytes)}
      , quads{}
    {}

    void thrash(RandomHelper &generator) {
      auto new_end = std::remove_if(
        begin(quads), end(quads), [&](auto&) { return generator.random_bool(); }
      );
      quads.erase(new_end, end(quads));
      std::vector<std::size_t> sizes{};
      for (auto &quad : quads) { sizes.push_back(quad.size_bytes()); }
      std::size_t bytes_used = std::accumulate(begin(sizes), end(sizes), 0);
      std::size_t current_max = generator.random_size(
        max_requested_memory_bytes - delta_bytes,
        max_requested_memory_bytes + delta_bytes
      );
      std::size_t headroom_bytes = current_max - bytes_used;
      std::size_t texture_bytes = generator.random_size(
        max_texture_bytes/2, max_texture_bytes
      );

      while (texture_bytes <= headroom_bytes) {
        auto level_zero_texels = level_zero_bytes(texture_bytes) / 4;
        std::size_t width = generator.random_size(10, level_zero_texels / 10);
        std::size_t height = level_zero_texels / width;
        quads.emplace_back(width, height, faker);
        headroom_bytes -= quads.back().size_bytes();
        texture_bytes = generator.random_size(
          max_texture_bytes/2, max_texture_bytes
        );
      }
    }

    void draw(RandomHelper &generator) const {
      for (auto const &quad : quads) {
        quad.draw(generator);
      }
    }

  private:
    std::size_t frame_count;
    std::size_t max_requested_memory_bytes;
    std::size_t delta_bytes;
    std::size_t max_texture_bytes;
    Faker faker;
    std::vector<RandomQuad> quads;
  };
}

#endif
