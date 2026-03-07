#include "include/schwarz_metric_ray.h"

SM_Ray::SM_Ray(Vec3 start, Vec3 direction, Vec3 black_hole_pos)
  : Ray{ start, direction }, black_hole_pos(black_hole_pos) {}

void SM_Ray::step(float dl) {
  Vec3 r = black_hole_pos - position;
  Vec3 normal = r.unit();

  float h2 = (r ^ direction).l2();

  // dv/ds = 3GMh^2 / c^2 r^4
  Vec3 a = 3 * constant::physics::G * constant::model::M / constant::physics::c / constant::physics::c
    * constant::physics::M_Sun  * h2 / r.l2() / r.l2()
    / 3.7e10f /* some magic number for calibrating the unit system */
    * normal;
  Vec3 dv = a * dl;

  direction = (direction + dv).unit();
  position += dl * direction;
}