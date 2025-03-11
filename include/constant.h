#ifndef CONSTANT_H__
#define CONSTANT_H__

constexpr float CONSTANT_PI = 3.14159265358979323846f;

constexpr float CONSTANT_G = 6.67430e-11f;
constexpr float CONSTANT_c = 299792458.f;
constexpr float CONSTANT_sigma = 5.670367e-8f;

constexpr float CONSTANT_M_Sun = 1.989e30f /* kg */;
constexpr float CONSTANT_ly = 9454254955488000.f /* m */;

constexpr float CONSTANT_M = 1.49e7f /* M_SUN */;
constexpr float CONSTANT_M_kg = CONSTANT_M * CONSTANT_M_Sun;
constexpr float CONSTANT_Rs = 0.00232f /* ly */;
constexpr float CONSTANT_Rs_m = 0.00232f * CONSTANT_ly /* ly */;

constexpr float CONSTANT_disk_thickness = 0.3 /* Rs */;
constexpr float CONSTANT_Rin = 3 /* Rs */;
constexpr float CONSTANT_Rout = 8 /* Rs */;

#endif