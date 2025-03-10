#ifndef RAY_H__
#define RAY_H__

#include "vec3.h"

class Ray {
public:
  Ray(Vec3 start, Vec3 end);
  Vec3 position(float t);

private:
  Vec3 start, end;
  Vec3 e;
};

#endif