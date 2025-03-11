#ifndef CONSTANT_H__
#define CONSTANT_H__

namespace Constant {
  constexpr float PI = 3.14159265358979323846f;

  constexpr float G = 6.67430e-11f;
  constexpr float c = 299792458.f;
  constexpr float sigma = 5.670367e-8f;

  constexpr float M_Sun = 1.989e30f /* kg */;
  constexpr float ly = 9454254955488000.f /* m */;

  constexpr float M = 1.49e7f /* M_SUN */;
  constexpr float M_kg = M * M_Sun;
  constexpr float Rs = 0.00232f /* ly */;
  constexpr float Rs_m = 0.00232f * ly /* m */;

  constexpr float disk_thickness = 0.3 /* Rs */;
  constexpr float Rin = 3 /* Rs */;
  constexpr float Rout = 8 /* Rs */;
};


#endif