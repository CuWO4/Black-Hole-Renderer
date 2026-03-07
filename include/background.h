#ifndef BACKGROUND_H__
#define BACKGROUND_H__

#include "vec3.hpp"
#include "constant.h"

#include "../lib/stb_image.h"

#include <string>
#include <assert.h>

class Background {
public:
  Background(std::string path) {
    int channel;
    texture = stbi_load(path.c_str(), &width, &height, &channel, channel_in_file);
    if (texture == nullptr) {
      fprintf(stderr, "failed to open file `%s`\n", path.c_str());
    }
  }

  ~Background() {
    if (texture != nullptr) {
      stbi_image_free(texture);
    }
  }

  Vec3 sample_equirect(Vec3 direction) {
    if (direction.l() < 1e-6) {
      return Vec3::black();
    }

    float theta = std::atan2(direction.y, direction.x);
    float phi = std::acos(direction.z / direction.l());

    float u = theta / constant::math::pi;
    float v = phi / constant::math::pi / 2;

    float fx = v * width;
    float fy = u * height;
    int x0 = std::min(static_cast<int>(std::floor(fx)), height - 1);
    int y0 = static_cast<int>(std::floor(fy)) % width;
    int x1 = std::min(x0 + 1, height - 1);
    int y1 = (y0 + 1) % width;

    float wx = fx - x0;
    float wy = fy - y0;

    Vec3 c00 = get_pixel(x0, y0);
    Vec3 c10 = get_pixel(x1, y0);
    Vec3 c01 = get_pixel(x0, y1);
    Vec3 c11 = get_pixel(x1, y1);

    return lerp(lerp(c00, c10, wx), lerp(c01, c11, wx), wy);
  }

private:
  int width, height;
  static constexpr int channel_in_file = 3;
  unsigned char* texture;

  Vec3 get_pixel(int x, int y) {
    int offset = (x * width + y) * channel_in_file;
    return Vec3(
      texture[offset] / 255.0f,
      texture[offset + 1] / 255.0f,
      texture[offset + 2] / 255.0f
    );
  }

  Vec3 lerp(Vec3 c0, Vec3 c1, float weight) {
    return (1 - weight) * c0 + weight * c1;
  }
};

#endif
