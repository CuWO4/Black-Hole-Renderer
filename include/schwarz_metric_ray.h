#ifndef SM_RAY__
#define SM_RAY__

#include "ray.h"

/**
 * @note  all units are Rs
 */
class SM_Ray: public Ray {
public:
  SM_Ray(Vec3 start, Vec3 direction, Vec3 black_hole_pos);
  void step(float dl) override;

private:
  Vec3 black_hole_pos;
};

#endif