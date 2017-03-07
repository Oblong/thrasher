#include <playback.hpp>

//#ifndef LOG_FILE
//  #define LOG_FILE "/tmp/gl_playback.cpp"
//#endif
//
//namespace {
//  class Playback {
//  public:
//    void create_texture(GLuint id) {
//    }
//
//    void mip_upload(GLuint id, GLint level, GLsizei height) {
//    }
//
//    void update_mip(GLuint id, GLint level, GLint xoffset GLint yoffset, GLsizei width, GLsizei height) {
//    }
//
//    void delete_texture(GLuint id) {
//    }
//
//    void swap() {
//    }
//  };
//}

int main() {
  bool result = freeze::openWindow(
    //1920 * 3, 1080, "THEFREEZE",
    100, 100, "THEFREEZE",
    [](auto swapBuffers) {
      glClearColor(1.0f, 0.0f, 0.0f, 1.0f);

      while (true) {
        glClear(GL_COLOR_BUFFER_BIT);

        swapBuffers();
      }

      return true;
    }
  );

  if (result) return 0; else return 1;
}
