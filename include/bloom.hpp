#ifndef BLOOM_HPP__
#define BLOOM_HPP__

#include "../utils/image.hpp"
#include "../utils/bar.hpp"

struct BloomConfig {
  BloomConfig(
    float superposition_coeff,
    int gaussian_kernel_size
  ):
    superposition_coeff(superposition_coeff),
    gaussian_kernel_size(gaussian_kernel_size) {}

  float superposition_coeff;
  int gaussian_kernel_size;
};

template <size_t Height, size_t Width, size_t Times>
void renderer_bloom(
  Image<Height, Width>& buffer,
  const Image<Height, Width>& origin,
  std::array<BloomConfig, Times> configures
);


static float brightness(Vec3 color) {
  static const Vec3 k(0.2126, 0.7152, 0.0722);
  return color % k;
}

template <size_t Height, size_t Width>
static void gaussian_blur(
  Image<Height, Width>& buffer, 
  const Image<Height, Width>& origin, 
  int kernel_size
) {
  float radius = kernel_size / 2.;

  Bar bar(Height + Width);

  static Image<Height, Width> tmp;

  for (int i = 0; i < Height; i++) {
    for (int j = 0; j < Width; j++) {
      Vec3 sum = Vec3::zero();
      float weight_sum = 0.0f;

      for (int k = -radius; k <= radius; ++k) {
        if (i + k < 0 || i + k >= Height) continue;

        float weight = std::exp(-k * k / (2.0f * radius * radius));
        sum += origin[i + k][j] * weight;
        weight_sum += weight;
      }

      tmp[i][j] = sum / weight_sum;
    }
    bar.step();
  }

  for (int j = 0; j < Width; j++) {
    for (int i = 0; i < Height; i++) {
      Vec3 sum = Vec3::zero();
      float weight_sum = 0.0f;

      for (int k = -radius; k <= radius; ++k) {
        if (j + k < 0 || j + k >= Width) continue;

        float weight = std::exp(-k * k / (2.0f * radius * radius));
        sum += tmp[i][j + k] * weight;
        weight_sum += weight;
      }

      buffer[i][j] = sum / weight_sum;
    }
    bar.step();
  }
}

template <size_t Height, size_t Width, size_t Times>
void renderer_bloom(
  Image<Height, Width>& buffer,
  const Image<Height, Width>& origin,
  std::array<BloomConfig, Times> configures
) {
  buffer = origin;

  static Image<Height, Width> highlight;
  for (int i = 0; i < Height; i++) {
    for (int j = 0; j < Width; j++) {
      highlight[i][j] = brightness(origin[i][j]) > 1.2
        ? origin[i][j]
        : Vec3::zero();
    }
  }

  for (const auto& config: configures) {
    static Image<Height, Width> after_blurred;
    gaussian_blur(after_blurred, highlight, config.gaussian_kernel_size);
    for (int i = 0; i < Height; i++) {
      for (int j = 0; j < Width; j++) {
        buffer[i][j] += after_blurred[i][j] * config.superposition_coeff;
      }
    }
  }
}

#endif