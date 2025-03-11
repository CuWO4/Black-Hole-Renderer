#include "include/schwarz_metric_ray.h"

SM_Ray::SM_Ray(Vec3 start, Vec3 direction, Vec3 black_hole_pos)
  : Ray{ start, direction }, black_hole_pos(black_hole_pos) {}

static float tan_small_angle(float angle) {
  return angle + angle * angle * angle / 3;
}

void SM_Ray::step(float dl) {
  Vec3 normal = (position - black_hole_pos).unit();
  float cos_theta = (normal ^ direction).l();

  float d_theta = 1.5f / Vec3::dis(position, black_hole_pos) * dl * cos_theta * cos_theta * cos_theta / 5.5 /* why... */;

  Vec3 dv = tan_small_angle(d_theta) * direction ^ (direction ^ normal) / cos_theta;
  
  position += dl * direction;
  direction = (direction + dv).unit();
}