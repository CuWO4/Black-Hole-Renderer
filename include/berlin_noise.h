#ifndef DISK_H__
#define DISK_H__

#include "vec3.hpp"

class BerlinNoise {
public:
  BerlinNoise(
    int layer_n,
    float frequency0,
    float detail_coef,
    float superposition_intensity,
    float contrast
  );

  float get_noise(Vec3 point);

private:
  int layer_n;
  float frequency0;
  float detail_coef;
  float superposition_intensity;
  float contrast;

  static float get_noise_one_layer(Vec3 point, float frequency);
};

#endif