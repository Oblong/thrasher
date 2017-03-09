#ifndef UUID_C7EAC68A_6ACA_478A_A771_3709196C154E
#define UUID_C7EAC68A_6ACA_478A_A771_3709196C154E

#include <random>

namespace forensics {
  class RandomHelper final {
  public:
    float random_float(float lower_bound, float upper_bound) {
      std::uniform_real_distribution<float> dist{lower_bound, upper_bound};
      return dist(mt);
    }

    // Uses signed char instead of unsigned to match GLbyte
    signed char random_byte() {
      // Apparently there are defects in the standard surrounding generating
      // random bytes. This is probably somewhat fishy, but it will have to do
      // for now.
      std::uniform_int_distribution<unsigned short> dist{
        std::numeric_limits<unsigned char>::min(),
        std::numeric_limits<unsigned char>::max()
      };
      return dist(mt);
    }

    bool random_bool() {
      std::bernoulli_distribution dist{0.5};
      return dist(mt);
    }

    std::size_t random_size(std::size_t lower_bound, std::size_t upper_bound) {
      std::uniform_int_distribution<std::size_t> dist{lower_bound, upper_bound};
      return dist(mt);
    }

  private:
    std::mt19937 mt{std::random_device{}()};
  };
}

#endif
