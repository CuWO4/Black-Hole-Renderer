#include "include/schwarz_metric_ray.h"

SM_Ray::SM_Ray(Vec3 start, Vec3 direction, Vec3 black_hole_pos)
  : Ray{ start, direction }, black_hole_pos(black_hole_pos) {}

void SM_Ray::step(float dl) {
  // r, position, dl are all in units of Rs (dimensionless length unit)
  Vec3 r = black_hole_pos - position;
  float r2 = r.l2();
  if (r2 < 1e-12f) {
    position += dl * direction;
    return;
  }
  Vec3 normal = r.unit();

  float h2 = (r ^ direction).l2();

  // In Rs-units: 3GM/c^2 = (3/2) * Rs
  Vec3 dv = (1.5f * h2 / (r2 * r2)) * normal * dl;

  direction = (direction + dv).unit();
  position += dl * direction;
}