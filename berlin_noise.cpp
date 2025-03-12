#include "include/berlin_noise.h"

#include <cmath>

// [0, 1] -> [0, 1]
static float smooth(float x) {
  return 3 * x * x - 2 * x * x * x;
}

// murmur hash
// -> [-1, 1)
static float rand_from_vec(Vec3 v) {
  float key = v.x + 0x80 * v.y + 0x8000 * v.z;
  float seed = 42;

  const uint8_t* data = reinterpret_cast<const uint8_t*>(&key);
  const int len = sizeof(float); // Size of float is 4 bytes

  uint32_t h1 = seed;

  const uint32_t c1 = 0xcc9e2d51;
  const uint32_t c2 = 0x1b873593;

  const uint32_t* blocks = reinterpret_cast<const uint32_t*>(data);
  for (int i = 0; i < len / 4; i++) {
    uint32_t k1 = blocks[i];

    k1 *= c1;
    k1 = (k1 << 15) | (k1 >> 17);
    k1 *= c2;

    h1 ^= k1;
    h1 = (h1 << 13) | (h1 >> 19);
    h1 = h1 * 5 + 0xe6546b64;
  }

  h1 ^= len;
  h1 ^= h1 >> 16;
  h1 *= 0x85ebca6b;
  h1 ^= h1 >> 13;
  h1 *= 0xc2b2ae35;
  h1 ^= h1 >> 16;

  return h1 / float(UINT32_MAX) - 0.5;
}

BerlinNoise::BerlinNoise(
    int layer_n,
    float frequency0,
    float detail_coef,
    float superposition_intensity,
    float contrast
):
  layer_n(layer_n), 
  frequency0(frequency0), 
  detail_coef(detail_coef), 
  superposition_intensity(superposition_intensity),
  contrast(contrast) {}

float BerlinNoise::get_noise(Vec3 point) {
  float res = 1;

  for (int i = 0; i < layer_n; i++) {
    float frequency = frequency0 * std::pow(detail_coef, i);
    res *= 1.f + superposition_intensity * get_noise_one_layer(point, frequency);
  }

  return std::log(1 + std::pow(res, contrast));
}

float BerlinNoise::get_noise_one_layer(Vec3 point, float frequency) {
  Vec3 transformed = point * frequency;

  Vec3 p0 = transformed.floor();
  Vec3 pf = transformed - p0;

  float v000 = rand_from_vec(p0);
  float v100 = rand_from_vec(p0 + Vec3(1, 0, 0));
  float v010 = rand_from_vec(p0 + Vec3(0, 1, 0));
  float v110 = rand_from_vec(p0 + Vec3(1, 1, 0));
  float v001 = rand_from_vec(p0 + Vec3(0, 0, 1));
  float v101 = rand_from_vec(p0 + Vec3(1, 0, 1));
  float v011 = rand_from_vec(p0 + Vec3(0, 1, 1));
  float v111 = rand_from_vec(p0 + Vec3(1, 1, 1));

  float v00 = v001 * smooth(pf.z) + v000 * smooth(1.0 - pf.z);
  float v10 = v101 * smooth(pf.z) + v100 * smooth(1.0 - pf.z);
  float v01 = v011 * smooth(pf.z) + v010 * smooth(1.0 - pf.z);
  float v11 = v111 * smooth(pf.z) + v110 * smooth(1.0 - pf.z);

  float v0 = v01 * smooth(pf.y) + v00 * smooth(1.0 - pf.y);
  float v1 = v11 * smooth(pf.y) + v10 * smooth(1.0 - pf.y);

  float v = v1 * smooth(pf.x) + v0 * smooth(1.0 - pf.x);

  return v;
}