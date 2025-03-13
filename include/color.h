#ifndef COLOR_H__
#define COLOR_H__

#include "vec3.hpp"

namespace color {
  float /* K */ get_disk_temperature(float r /* m */);
  float /* K */ maximum_temperature();
  Vec3 black_body_color(float temperature /* K */);
}

#endif