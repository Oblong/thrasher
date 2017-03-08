#ifndef UUID_C7EAC68A_6ACA_478A_A771_3709196C154E
#define UUID_C7EAC68A_6ACA_478A_A771_3709196C154E

#include <random>
#include <GL/gl.h>

namespace forensics {
  class RandomFloat final {
  public:
    RandomFloat(float low, float high)
    : mt{std::random_device{}()}, dist{low, high} {}
    float operator()() { return dist(mt); }
  private:
    std::mt19937 mt;
    std::uniform_real_distribution<float> dist;
  };

  class RandomByte final {
  public:
    GLbyte operator()() { return dist(mt); }
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

  class RandomBool final {
  public:
    RandomBool(double percent)
    : mt{std::random_device{}()}, dist{percent} {}
    bool operator()() { return dist(mt); }
  private:
    std::mt19937 mt;
    std::bernoulli_distribution dist;
  };

  class RandomSize final {
  public:
    std::size_t operator()(std::size_t lower_bound, std::size_t upper_bound) {
      std::uniform_int_distribution<std::size_t> dist{lower_bound, upper_bound};
      return dist(mt);
    }
  private:
    std::mt19937 mt{std::random_device{}()};
  };
}

#endif
