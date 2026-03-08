#ifndef RNG_H__
#define RNG_H__

#include <cstdint>

struct XorShift32Rng {
  uint32_t state;
  explicit XorShift32Rng(uint32_t seed = 1u) : state(seed ? seed : 1u) {}

  uint32_t next_u32() {
    uint32_t x = state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    state = x;
    return x;
  }

  // [0,1)
  float next_f01() {
    // 24-bit mantissa-ish
    return (next_u32() >> 8) * (1.0f / 16777216.0f);
  }
};

#endif