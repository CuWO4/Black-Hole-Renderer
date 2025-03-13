#include "include/color.h"

#include "include/constant.h"

#include <cmath>

using namespace constant;

namespace color {
  float /* K */ get_disk_temperature(float r /* m */) {
    float accretion_rate = 2e-6 * 6.327 * 1.732 * model::M_kg / physics::c / physics::c;
    return model::temperature_coeff 
    * std::pow(
      3. * physics::G * model::M_kg * accretion_rate  / 
      (r * physics::sigma * r * r * 8 * math::pi),
      0.25
    )
    * pow((1. - std::sqrt(model::Rs_m / r * model::Rin)), 0.25)
    * exp(-.025 * r / model::Rs_m);
  }

  float /* K */ maximum_temperature() {
    static float value = 0;
    [[unlikely]] if (!value) {
      value = get_disk_temperature(3.5 * model::Rs_m);
    }
    return value;
  }


  Vec3 black_body_color(float temperature /* K */) {
    const float k[] = { 2.05539304, 2.63463675, 3.30145739 };
    if (temperature < 120.0) { temperature = 120.0; }
    Vec3 color;
    for (int i = 0; i < 3; i++) {
      color[i] = std::exp(k[i] * (temperature - 6500.0) / (temperature * 1.43));
    }
    color /= std::max(color.x, std::max(color.y, color.z));
    return color;
  }

}