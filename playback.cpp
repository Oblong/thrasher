#include <playback.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <random>
#include <unordered_map>
#include <GL/gl.h>

//#ifndef LOG_FILE
//  #define LOG_FILE "/tmp/gl_playback.cpp"
//#endif

namespace {
  namespace detail {
    class RandFloat final {
    public:
      RandFloat(std::random_device &rd) : mt{rd()} {}
      float operator()() {
        return dist(mt);
      }
    private:
      std::mt19937 mt;
      std::uniform_real_distribution<float> dist{0.0f, 1.0f};
    };

    class Filler final {
    public:
      Filler(RandFloat &generator) : r{generator()}, g{generator()}, b{generator()}, a{generator()} {}

      float operator()() {
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
        return 0.0f;
      }

    private:
      std::size_t count = 0;
      float r, g, b, a;
    };

    template <std::size_t max_texture_bytes>
    class BufferFaker {
    public:
      BufferFaker(detail::RandFloat &color_generator_) : color_generator{color_generator_} {}

      template <typename Callback>
      void recolor(std::size_t size, Callback callback) {
        if (size > max_texture_bytes) {
          fprintf(stderr, "tried to fake a texture that is too big\n");
          return;
        }

        auto data = texture_buffer->data();
        std::generate(texture_buffer->begin(), texture_buffer->begin() + size, detail::Filler{color_generator});

        callback(texture_buffer->data());
      }

    private:
      detail::RandFloat &color_generator;
      using Buffer = std::array<GLbyte, max_texture_bytes>;
      std::unique_ptr<Buffer> texture_buffer = std::make_unique<Buffer>();
    };

    class TextureHandles {
    public:
      TextureHandles() = default;
      TextureHandles(TextureHandles const &) = delete;
      TextureHandles(TextureHandles&&) = default;
      TextureHandles &operator=(TextureHandles const &) = delete;
      TextureHandles &operator=(TextureHandles&&) = default;
      ~TextureHandles() {
        for (auto handle : handles) {
          if (handle.second != 0) {
            glDeleteTextures(1, &handle.second);
          }
        }
      }

      auto begin() const { return handles.begin(); }
      auto end() const { return handles.end(); }

      auto erase(std::size_t id) { handles.erase(id); }

      GLuint &operator[](std::size_t id) { return handles[id]; }

    private:
      std::unordered_map<std::size_t, GLuint> handles{};
    };
  }

  template <typename Swapper>
  class Playback final {
    static constexpr std::size_t max_texture_bytes = 101782080;
    static constexpr std::size_t bytes_per_texel = 4;

  public:
    Playback(std::random_device &rd, Swapper swap_buffers_)
      : texture_handles{}
      , swap_buffers{std::move(swap_buffers_)}
      , float_generator{rd}
      , faker{float_generator}
    {}

    void create_texture(std::size_t id) {
      glGenTextures(1, &texture_handles[id]);
    }

    void mip_reserve(std::size_t id, GLint level, GLsizei width, GLsizei height) {
      auto size = width * height * bytes_per_texel;
      if (size > max_texture_bytes) {
        fprintf(stderr, "texture is too big!\n");
        return;
      }

      mip_helper(id, level, width, height, nullptr);
    }

    void mip_upload(std::size_t id, GLint level, GLsizei width, GLsizei height) {
      auto size = width * height * bytes_per_texel;
      if (size > max_texture_bytes) {
        fprintf(stderr, "texture is too big!\n");
        return;
      }

      faker.recolor(size, [=](auto data) {
        this->mip_helper(id, level, width, height, data);
      });
    }

    void update_mip(std::size_t id, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height) {
      auto size = width * height * bytes_per_texel;
      if (size > max_texture_bytes) {
        fprintf(stderr, "texture is too big!\n");
        return;
      }

      faker.recolor(size, [=](auto data) {
        glTexSubImage2D(GL_TEXTURE_2D, level, xoffset, yoffset, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
      });
    }

    void delete_texture(std::size_t id) {
      glDeleteTextures(1, &texture_handles[id]);
      texture_handles.erase(id);
    }

    void draw() {
      for (auto pair : texture_handles) {
        GLuint handle = pair.second;
        if (handle != 0) {
          draw_rectangle(handle);
        }
      }

      swap_buffers();
    }

  private:
    void draw_rectangle(GLuint handle) {
      // Bind Texture
      glBindTexture (GL_TEXTURE_2D, handle);

      // Render Settings
      glEnable(GL_TEXTURE_2D);
      //glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
      //glColor3ub(255,255,255);

      glBegin(GL_QUADS);

      float left = float_generator();
      float right = float_generator();
      float top = float_generator();
      float bottom = float_generator();

      // Top Left
      glTexCoord2f(0.0f, 1.0f);
      glVertex2f(left, bottom);

      // Top Right
      glTexCoord2f(1.0f, 1.0f);
      glVertex2f(right, bottom);

      // Bottom Right
      glTexCoord2f(1.0f, 0.0f);
      glVertex2f(right, top);

      // Bottom Left
      glTexCoord2f(0.0f, 0.0f);
      glVertex2f(left, top);

      glEnd();
    }

    void mip_helper(std::size_t id, GLint level, GLsizei width, GLsizei height, void *data) {
      glEnable(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, texture_handles[id]);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      glTexImage2D(GL_TEXTURE_2D, level, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    }

    detail::TextureHandles texture_handles;
    Swapper swap_buffers;
    detail::RandFloat float_generator;
    detail::BufferFaker<max_texture_bytes> faker;
  };

  template <typename Swapper>
  Playback<Swapper> make_playback(std::random_device &rd, Swapper swapper) {
    return {rd, std::move(swapper)};
  }
}

int main() {
  bool result = freeze::openWindow(
    //1920 * 3, 1080, "THEFREEZE",
    100, 100, "THEFREEZE",
    [](auto swap_buffers) {
      std::random_device rd{};

      glClearColor(1.0f, 0.0f, 0.0f, 1.0f);

      while (true) {
        auto playback = make_playback(rd, std::move(swap_buffers));

        glClear(GL_COLOR_BUFFER_BIT);

        playback.create_texture(1);
        playback.mip_upload(1, 0, 8496, 2396);
        playback.mip_upload(1, 1, 4248, 1198);
        playback.mip_upload(1, 2, 2124, 599);
        playback.mip_upload(1, 3, 1062, 299);
        playback.mip_upload(1, 4, 531, 149);
        playback.mip_upload(1, 5, 265, 74);
        playback.mip_upload(1, 6, 132, 37);
        playback.mip_upload(1, 7, 66, 18);
        playback.mip_upload(1, 8, 33, 9);
        playback.mip_upload(1, 9, 16, 4);
        playback.mip_upload(1, 10, 8, 2);
        playback.mip_upload(1, 11, 4, 1);

        playback.draw();
      }

      return true;
    }
  );

  if (result) return 0; else return 1;
}
