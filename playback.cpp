#include <playback.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <random>
#include <thread>
#include <unordered_map>
#include <GL/gl.h>

//#ifndef LOG_FILE
//  #define LOG_FILE "/tmp/gl_playback.cpp"
//#endif

namespace {
  namespace detail {
    class RandFloat final {
    public:
      RandFloat(float low, float high) : mt{std::random_device{}()}, dist{low, high} {}
      float operator()() {
        return dist(mt);
      }
    private:
      std::mt19937 mt;
      std::uniform_real_distribution<float> dist;
    };

    class RandByte final {
    public:
      GLbyte operator()() {
        return dist(mt);
      }
    private:
      // Apparently there are defects in the standard surrounding generating
      // random bytes. This is probably somewhat fishy, but it will have to do
      // for now.
      std::mt19937 mt{std::random_device{}()};
      std::uniform_int_distribution<unsigned short> dist{
        std::numeric_limits<unsigned char>::min(),
        std::numeric_limits<unsigned char>::max()
      };
    };

    class Filler final {
    public:
      Filler(RandByte &generator) : r{generator()}, g{generator()}, b{generator()}, a{generator()} {}

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
    class BufferFaker {
    public:
      template <typename Callback>
      void recolor(std::size_t size, Callback callback) {
        if (size > max_texture_bytes) {
          fprintf(stderr, "tried to fake a texture that is too big\n");
          return;
        }

        std::generate(texture_buffer->begin(), texture_buffer->begin() + size, detail::Filler{color_generator});

        callback(texture_buffer->data());
      }

    private:
      detail::RandByte color_generator{};
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
        : raii_handle{0}
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
    private:
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

    private:
      RandFloat float_generator;
      FakeTexture<max_texture_bytes> texture;
    };
  }

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
    std::unordered_map<std::size_t, detail::RandomQuad<max_texture_bytes>> quads;
    Swapper const &swap_buffers;
    detail::BufferFaker<max_texture_bytes> faker;
  };

  template <typename Swapper>
  Playback<Swapper> make_playback(Swapper const &swapper) {
    return {swapper};
  }
}

int main() {
  bool result = freeze::openWindow(
    //1920 * 3, 1080, "THEFREEZE",
    500, 500, "THEFREEZE",
    [](auto swap_buffers) {
      glClearColor(1.0f, 0.0f, 0.0f, 1.0f);

      while (true) {
        auto playback = make_playback(swap_buffers);

        glClear(GL_COLOR_BUFFER_BIT);

        playback.create_quad(1, 132, 37);

        playback.draw();

        using namespace std::chrono_literals;
        std::this_thread::sleep_for(0.5s);
      }

      return true;
    }
  );

  if (result) return 0; else return 1;
}
