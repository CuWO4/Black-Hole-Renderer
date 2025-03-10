#include "include/ray.h"

Ray::Ray(Vec3 start, Vec3 direction)
  : start{start}, direction{direction.unit()}, position{start} {}

void Ray::step(float dl) {
  position += dl * direction;
}

Vec3 Ray::get_position() { 
  return position;
}
