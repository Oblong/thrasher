#ifndef UUID_F18C895E_08D7_466A_BA71_BD328E26DCA9
#define UUID_F18C895E_08D7_466A_BA71_BD328E26DCA9

#include <GL/gl.h>
#include <GLFW/glfw3.h>
#include <cstdio>
#include <memory>

namespace thrasher {
  namespace detail {
    extern "C" inline void error_callback(int error, const char* description) {
      printf("glfw error: 0x%x, desc: %s\n", error, description);
    }

    extern "C" inline void window_size_callback(GLFWwindow *, int width, int height) {
      printf("w: %d, h: %d\n", width, height);
      glViewport(0, 0, width, height);
    }

    class GLFWWrapper {
    public:
      GLFWWrapper() : loaded{glfwInit() != 0} {
        if (!loaded) fprintf(stderr, "Failed to load GLFW\n");

        glfwSetErrorCallback(&detail::error_callback);
      }
      ~GLFWWrapper() { if (loaded) glfwTerminate(); }
      GLFWWrapper(GLFWWrapper const&) = delete;
      GLFWWrapper(GLFWWrapper&&) = delete;
      GLFWWrapper& operator=(GLFWWrapper const&) = delete;
      GLFWWrapper& operator=(GLFWWrapper&&) = delete;

      explicit operator bool() const { return loaded; }
    private:
      bool loaded;
    };
  }

  template <typename Callback>
  inline bool openWindow(
    int width,
    int height,
    bool double_buffer,
    char const *title,
    Callback callback
  ) {
    static detail::GLFWWrapper wrapper{};
    if (!static_cast<bool>(wrapper)) return false;

    glfwWindowHint(GLFW_DOUBLEBUFFER, double_buffer);

    std::unique_ptr<GLFWwindow, decltype(&glfwDestroyWindow)> window{
      glfwCreateWindow(width, height, title, nullptr, nullptr),
      &glfwDestroyWindow
    };

    if (nullptr == window) return false;

    glfwMakeContextCurrent(window.get());

    // Do our best to keep the viewport sane
    glfwSetWindowSizeCallback(window.get(), &detail::window_size_callback);
    glfwGetWindowSize(window.get(), &width, &height);
    glViewport(0, 0, width, height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);

    return callback([window = std::move(window)] {
      glfwSwapBuffers(window.get());
    });
  }
}

#endif
