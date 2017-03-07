// Compile like this:
// g++ -std=c++14 -fPIC -shared -o teximage-preload.so teximage-preload.c -ldl
// Add this to /lib/systemd/system/siemcy.service:
// Environment=LD_PRELOAD=/path/to/teximage-preload.so

#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glx.h>

#include <dlfcn.h>
#include <stdio.h>

#include <unordered_map>
#include <memory>

#ifndef LOG_FILE
  #define LOG_FILE "/tmp/gl_playback.cpp"
#endif

namespace {
  template <typename FunctionPointer>
  class DynamicLoader {
  public:
    DynamicLoader(char const *name_)
      : name{name_}
        // UNDEFINED BEHAVIOR!!!
      , function_pointer{reinterpret_cast<FunctionPointer>(reinterpret_cast<long>(dlsym(RTLD_NEXT, name)))}
    {}

    template <typename Callback>
    void operator()(Callback callback) {
      if (nullptr == function_pointer) {
        fprintf (stderr, "ERROR: Unable to find the real %s\n", name);
        return;
      }

      callback(*function_pointer);
    }

  private:
    char const *name;
    FunctionPointer function_pointer;
  };

  class Logger final {
  public:
    Logger(char const *path = LOG_FILE)
      : handle{fopen(path, "a"), &fclose}
    {
      if (nullptr == handle) fprintf(stderr, "ERROR: Failed to open log file\n");
    }

    template <typename ...Args>
    void write(Args&&... args) const {
      if (nullptr == handle) return;
      #pragma GCC diagnostic push
      #pragma GCC diagnostic ignored "-Wformat-security"
      fprintf(handle.get(), std::forward<Args>(args)...);
      #pragma GCC diagnostic pop
      fflush(handle.get());
    }

  private:
    std::unique_ptr<FILE, decltype(&fclose)> handle;
  };

  class Inspector final {
  private:
    static GLint current_texture_id() {
      GLint id;
      glGetIntegerv(GL_TEXTURE_BINDING_2D, &id);
      return id;
    }

  public:
    static auto& instance() {
      static Inspector singleton{};
      return singleton;
    }

    template <typename Callback>
    void onTexImage2D (
      Callback callback,
      GLenum target, GLint level, GLint internalFormat, GLsizei width,
      GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *data
    ) {
      if (nullptr != data) {
        auto id = current_texture_id();

        stuff_happened = true;
        if (!tex_ids[id]) {
          log.write("playback.create_texture(%d);\n", id);
        }
        tex_ids[id] = true;

        log.write("playback.mip_upload(%d, %d, %d, %d);\n", id, level, width, height);
      }
      callback(target, level, internalFormat, width, height, border, format, type, data);
    }

    template <typename Callback>
    void onTexSubImage2D (
      Callback callback,
      GLenum target, GLint level, GLint xoffset, GLint yoffset,
      GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *data
    ) {
      stuff_happened = true;
      log.write("playback.update_mip(%d, %d, %d, %d, %d, %d)", current_texture_id(), level, xoffset, yoffset, width, height);
      callback(target, level, xoffset, yoffset, width, height, format, type, data);
    }

    template <typename Callback>
    void onDeleteTextures (Callback callback, GLsizei n, const GLuint *texture_ids) {
      stuff_happened = true;
      for (int i = 0; i < n; i++) {
        auto id = texture_ids[i];
        log.write("playback.delete_texture(%d);\n", n, id);
        tex_ids[id] = false;
      }

      callback(n, texture_ids);
    }

    template <typename Callback>
    void onSwap (Callback callback, Display *dpy, GLXDrawable drawable) {
      if (stuff_happened) {
        log.write("playback.swap();\n");
      }
      callback(dpy, drawable);
      stuff_happened = GL_FALSE;
    }

  private:
    Inspector() = default;

    bool stuff_happened = false;
    std::array<bool, 10000> tex_ids{};
    Logger log{};
  };
}

#define DYNAMIC_LOADER(name) ::DynamicLoader<decltype(&name)>{#name}

// Wrap glEnable function call
extern "C" void glTexImage2D (
  GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height,
  GLint border, GLenum format, GLenum type, const GLvoid *data
) {
  static auto loader = DYNAMIC_LOADER(glTexImage2D);
  loader([=](auto callback) {
    Inspector::instance().onTexImage2D(
      callback, target, level, internalFormat, width, height, border, format, type, data
    );
  });
}

extern "C" void glTexSubImage2D (
  GLenum target, GLint level, GLint xoffset, GLint yoffset,
  GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *data
) {
  static auto loader = DYNAMIC_LOADER(glTexSubImage2D);
  loader([=](auto callback) {
    Inspector::instance().onTexSubImage2D(
      callback, target, level, xoffset, yoffset, width, height, format, type, data
    );
  });
}

extern "C" void glXSwapBuffers (Display *dpy, GLXDrawable drawable) {
  static auto loader = DYNAMIC_LOADER(glXSwapBuffers);
  loader([=](auto callback) {
    Inspector::instance().onSwap(callback, dpy, drawable);
  });
}

extern "C" void glDeleteTextures (GLsizei n, const GLuint *texture_ids) {
  static auto loader = DYNAMIC_LOADER(glDeleteTextures);
  loader([=](auto callback) {
    Inspector::instance().onDeleteTextures(callback, n, texture_ids);
  });
}
