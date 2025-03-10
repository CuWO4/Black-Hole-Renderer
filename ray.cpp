#include "include/ray.h"

Ray::Ray(Vec3 start, Vec3 end)
  : start{start}, end{end}, e{(end - start).unit()} {}

Vec3 Ray::position(float t) {
  return start + t * e;
}
