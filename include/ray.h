#ifndef RAY_H__
#define RAY_H__

#include "vec3.hpp"

class Ray {
public:
  Ray(Vec3 start, Vec3 direction);
  virtual void step(float dl);
  Vec3 get_position();

public:
  Vec3 start, direction;
  Vec3 position;
};

#endif