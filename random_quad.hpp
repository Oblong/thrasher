#ifndef UUID_1E48FB08_4CBB_4468_8689_1DA8587E48D3
#define UUID_1E48FB08_4CBB_4468_8689_1DA8587E48D3

#include <random_helper.hpp>

#include <GL/gl.h>

#include <algorithm>
#include <array>
#include <memory>

namespace thrasher {
  class Filler final {
  public:
    Filler(RandomHelper &generator)
      : r{generator.random_byte()}
      , g{generator.random_byte()}
      , b{generator.random_byte()}
      , a{generator.random_byte()} {}

    GLbyte operator()() {
      auto mod = count % 4;
      ++count;
      if (mod == 0) {
        return r;
      } else if (mod == 1) {
        return g;
      } else if (mod == 2) {
        return b;
      } else if (mod == 3) {
        return a;
      }
      fprintf(stderr, "Color filler failure\n");
      return 0x00;
    }

  private:
    std::size_t count = 0;
    GLbyte r, g, b, a;
  };

  class SharedBufferFaker final {
  public:
    SharedBufferFaker(RandomHelper &color_generator_, std::size_t max_texture_bytes)
      : color_generator{color_generator_}
      , texture_buffer(max_texture_bytes)
    {}

    template <typename Callback>
    void recolor(std::size_t size, Callback callback) {
      if (size > texture_buffer.size()) {
        fprintf(stderr, "Tried to fake a texture of size %lu (max is %lu)\n", size, texture_buffer.size());
        return;
      }

      std::generate(texture_buffer.begin(), texture_buffer.begin() + size, Filler{color_generator});

      callback(texture_buffer.data());
    }

  private:
    RandomHelper &color_generator;
    std::vector<GLbyte> texture_buffer;
  };

  class UniqueBufferFaker final {
  public:
    UniqueBufferFaker(RandomHelper &color_generator_, std::size_t max_texture_bytes)
      : color_generator{color_generator_}
      , max_texture_bytes{max_texture_bytes}
    {}

    template <typename Callback>
    void recolor(std::size_t size, Callback callback) {
      if (size > max_texture_bytes) {
        fprintf(stderr, "Tried to fake a texture of size %lu (max is %lu)\n", size, max_texture_bytes);
        return;
      }

      std::vector<GLbyte> buffer(size);
      std::generate(begin(buffer), end(buffer), Filler{color_generator});

      callback(buffer.data());
    }

  private:
    RandomHelper &color_generator;
    std::size_t max_texture_bytes;
  };

  class TextureHandle final {
  public:
    TextureHandle() : handle{0} { glGenTextures(1, &handle); }
    TextureHandle(TextureHandle const&) = delete;
    TextureHandle(TextureHandle && other) noexcept : handle{other.handle} {
      other.handle = 0;
    }
    TextureHandle &operator=(TextureHandle const&) = delete;
    TextureHandle &operator=(TextureHandle && other) noexcept {
      if (this == &other) return *this;
      if (0 != handle) glDeleteTextures(1, &handle);
      handle = other.handle;
      other.handle = 0;
      return *this;
    }
    ~TextureHandle() {
      if (0 != handle) glDeleteTextures(1, &handle);
    }

    explicit operator bool() const {
      return handle != 0;
    }

    GLuint get() const {
      return handle;
    }
  private:
    GLuint handle;
  };

  class FakeTexture final {
    static constexpr std::size_t bytes_per_texel = 4;
  public:
    template <typename Faker, typename OnSuccess, typename OnFailure>
    static auto create(
      GLsizei width, GLsizei height, Faker &faker,
      OnSuccess on_success, OnFailure on_failure
    ) {
      GLsizei texture_size = 0;
      TextureHandle handle{};
      if (!handle) return on_failure();

      GLsizei num_mips = std::log2(std::min(width, height));
      glEnable(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, handle.get());
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, num_mips);

      for (GLsizei level = 0; level <= num_mips; ++level) {
        auto size = width * height * bytes_per_texel;
        texture_size += size;
        faker.recolor(size, [=](auto data) {
          glTexImage2D(GL_TEXTURE_2D, level, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        });
        width /= 2;
        height /= 2;
      }

      bool error = false;
      while (glGetError() != GL_NO_ERROR) { error = true; }
      if (error) return on_failure();

      return on_success(FakeTexture{texture_size, std::move(handle)});
    }

    GLuint handle() const { return raii_handle.get(); }

    explicit operator bool() const {
      return static_cast<bool>(raii_handle);
    }

    GLsizei size_bytes() const {
      if (!raii_handle) return 0;
      return texture_size;
    }

  private:
    FakeTexture(GLsizei texture_size_, TextureHandle raii_handle_)
      : texture_size{texture_size_}, raii_handle{std::move(raii_handle_)} {}

    GLsizei texture_size;
    TextureHandle raii_handle;
  };

  class RandomQuad final {
  public:
    template <typename Faker, typename OnSuccess, typename OnFailure>
    static auto create(
      GLsizei width, GLsizei height, Faker &faker,
      OnSuccess on_success, OnFailure on_failure
    ) {
      return FakeTexture::create(
        width, height, faker,
        [=](FakeTexture texture) { return on_success(RandomQuad{std::move(texture)}); },
        on_failure
      );
    }

    explicit operator bool() const {
      return static_cast<bool>(texture);
    }

    void draw(RandomHelper &generator) const {
      if (!texture) return;

      glEnable(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, texture.handle());

      glBegin(GL_QUADS);

      float left = generator.random_float(-1.0f, 1.0f);
      float right = generator.random_float(-1.0f, 1.0f);
      float top = generator.random_float(-1.0f, 1.0f);
      float bottom = generator.random_float(-1.0f, 1.0f);

      glTexCoord2f(0.0f, 0.0f);
      glVertex2f(left, bottom);

      glTexCoord2f(1.0f, 0.0f);
      glVertex2f(right, bottom);

      glTexCoord2f(1.0f, 1.0f);
      glVertex2f(right, top);

      glTexCoord2f(0.0f, 1.0f);
      glVertex2f(left, top);

      glEnd();
    }

    std::size_t size_bytes() const {
      return texture.size_bytes();
    }

  private:
    RandomQuad(FakeTexture texture)
      : texture{std::move(texture)}
    {}

    FakeTexture texture;
  };
}

#endif
