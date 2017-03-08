#ifndef UUID_1E48FB08_4CBB_4468_8689_1DA8587E48D3
#define UUID_1E48FB08_4CBB_4468_8689_1DA8587E48D3

#include <random_helper.hpp>

#include <GL/gl.h>

#include <algorithm>
#include <array>
#include <memory>

namespace forensics {
  class Filler final {
  public:
    Filler(RandomByte &generator) : r{generator()}, g{generator()}, b{generator()}, a{generator()} {}

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

  template <std::size_t max_texture_bytes>
  class BufferFaker final {
  public:
    template <typename Callback>
    void recolor(std::size_t size, Callback callback) {
      if (size > max_texture_bytes) {
        fprintf(stderr, "Tried to fake a texture of size %lu (max is %lu)\n", size, max_texture_bytes);
        return;
      }

      std::generate(texture_buffer->begin(), texture_buffer->begin() + size, Filler{color_generator});

      callback(texture_buffer->data());
    }

  private:
    RandomByte color_generator{};
    using Buffer = std::array<GLbyte, max_texture_bytes>;
    std::unique_ptr<Buffer> texture_buffer = std::make_unique<Buffer>();
  };

  class TextureHandle final {
  public:
    TextureHandle(GLuint handle_) : handle{handle_} {}
    TextureHandle(TextureHandle const&) = delete;
    TextureHandle(TextureHandle && other) : handle{other.handle} {
      other.handle = 0;
    }
    TextureHandle &operator=(TextureHandle const&) = delete;
    TextureHandle &operator=(TextureHandle && other) {
      handle = other.handle;
      other.handle = 0;
      return *this;
    }
    ~TextureHandle() {
      if (0 != handle) glDeleteTextures(1, &handle);
    }

    operator bool() const {
      return handle != 0;
    }

    GLuint get() const {
      return handle;
    }
  private:
    GLuint handle;
  };

  template <std::size_t max_texture_bytes>
  class FakeTexture final {
    static constexpr std::size_t bytes_per_texel = 4;
  public:
    FakeTexture(GLsizei width, GLsizei height, BufferFaker<max_texture_bytes> &faker)
      : texture_size{0}
      , raii_handle{0}
    {
      GLuint handle;

      glGenTextures(1, &handle);
      if (0 == handle) return;

      GLsizei num_mips = std::log2(std::min(width, height));
      glEnable(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, handle);
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

      raii_handle = handle;
    }

    GLuint handle() const { return raii_handle.get(); }

    operator bool() const {
      return raii_handle;
    }

    GLsizei size_bytes() const {
      return texture_size;
    }

  private:
    GLsizei texture_size;
    TextureHandle raii_handle;
  };

  template <std::size_t max_texture_bytes>
  class RandomQuad final {
  public:
    RandomQuad(GLsizei width, GLsizei height, BufferFaker<max_texture_bytes> &faker)
      : texture{width, height, faker}
      , float_generator{-1.0f, 1.0f}
    {}

    operator bool() const {
      return texture;
    }

    void draw() {
      if (!texture) return;

      glEnable(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, texture.handle());

      glBegin(GL_QUADS);

      float left = float_generator();
      float right = float_generator();
      float top = float_generator();
      float bottom = float_generator();

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
    RandomFloat float_generator;
    FakeTexture<max_texture_bytes> texture;
  };
}

#endif
