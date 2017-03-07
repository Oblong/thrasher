#ifndef RUNNER_H
#define RUNNER_H

#include <GL/gl.h>
#include <GLFW/glfw3.h>
#include <cstdio>
#include <memory>

namespace freeze {
  namespace detail {
    class GLFWWrapper {
    public:
      GLFWWrapper() : loaded{glfwInit() != 0} {
        if (!loaded) fprintf(stderr, "Failed to load GLFW");
      }
      ~GLFWWrapper() { if (loaded) glfwTerminate(); }
      GLFWWrapper(GLFWWrapper const&) = delete;
      GLFWWrapper(GLFWWrapper&&) = delete;
      GLFWWrapper& operator=(GLFWWrapper const&) = delete;
      GLFWWrapper& operator=(GLFWWrapper&&) = delete;

      operator bool() { return loaded; }
    private:
      int loaded;
    };
  }

  template <typename Callback>
  inline bool openWindow(int width, int height, char const *title, Callback callback) {
    static detail::GLFWWrapper wrapper{};
    if (!wrapper) return false;

    std::unique_ptr<GLFWwindow, decltype(&glfwDestroyWindow)> window{
      glfwCreateWindow(width, height, title, NULL, NULL),
      &glfwDestroyWindow
    };

    if (nullptr == window) return false;

    glfwMakeContextCurrent(window.get());

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
