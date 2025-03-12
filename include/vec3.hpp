#ifndef VEC3_HPP__
#define VEC3_HPP__

#include <iostream>

class Vec3 {
public:
  Vec3(float x = 0.f, float y = 0.f, float z = 0.f);

  Vec3& operator+();
  Vec3 operator-();
  float operator[](int i) const;
  float& operator[](int i);

  Vec3 operator+(Vec3 v);
  Vec3 operator-(Vec3 v);
  Vec3 operator*(Vec3 v);
  Vec3 operator/(Vec3 v);
  Vec3 operator*(float t);
  Vec3 operator/(float t);
  friend Vec3 operator*(float t,Vec3 v);
  friend Vec3 operator/(float t,Vec3 v);
  Vec3& operator+=(Vec3 v);
  Vec3& operator-=(Vec3 v);
  Vec3& operator*=(Vec3 v);
  Vec3& operator/=(Vec3 v);
  Vec3& operator*=(float t);
  Vec3& operator/=(float t);

  bool operator==(Vec3 v);
  bool operator!=(Vec3 v);

  float operator%(Vec3 v); /* dot product   */
  Vec3 operator^(Vec3 v);  /* cross product */

  float l();
  float l2();
  Vec3 unit();
  static float dis(Vec3 v1, Vec3 v2);

  Vec3 proj_to(Vec3 to);
  static float cos_angle_of(Vec3 v1, Vec3 v2);
  static float angle(Vec3 v1, Vec3 v2);

  Vec3 reflect(Vec3 normal);

  Vec3 abs();
  Vec3 floor();
  Vec3 ceil();

  static Vec3 random_unit_vec();

public:
  union {
    struct { float x, y, z; };
    struct { float r, g, b; }; /* [0, 1] */
    float e[3];
  };

  int R(); /* [0, 255] */
  int G(); /* [0, 255] */
  int B(); /* [0, 255] */

  Vec3 rgb_to_hsv(Vec3 rgb);
  Vec3 hsv_to_rgb(Vec3 hsv);

public:
  friend std::ostream& operator<<(std::ostream& out, Vec3 v);

public:
  static Vec3 zero() { return Vec3(0, 0, 0); }
  static Vec3 one() { return Vec3(1, 1, 1); }

  static Vec3 origin() { return Vec3(0, 0, 0); }
  static Vec3 i_e() { return Vec3(1, 0, 0); }
  static Vec3 j_e() { return Vec3(0, 1, 0); }
  static Vec3 k_e() { return Vec3(0, 0, 1); }

  static Vec3 black() { return Vec3(0, 0, 0); }
  static Vec3 red() { return Vec3(1, 0, 0); }
  static Vec3 green() { return Vec3(0, 1, 0); }
  static Vec3 blue() { return Vec3(0, 0, 1); }
  static Vec3 white() { return Vec3(1, 1, 1); }
};

#include "constant.h"

#include <cmath>
#include <algorithm>

inline Vec3::Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

inline Vec3& Vec3::operator+() { return *this; }
inline Vec3 Vec3::operator-() { return Vec3(-x, -y, -z); }
inline float Vec3::operator[](int i) const { return e[i]; }
inline float& Vec3::operator[](int i) { return e[i]; }

inline Vec3 Vec3::operator+(Vec3 v) { return Vec3(x + v.x, y + v.y, z + v.z); }
inline Vec3 Vec3::operator-(Vec3 v) { return Vec3(x - v.x, y - v.y, z - v.z); }
inline Vec3 Vec3::operator*(Vec3 v) { return Vec3(x * v.x, y * v.y, z * v.z); }
inline Vec3 Vec3::operator/(Vec3 v) { return Vec3(x / v.x, y / v.y, z / v.z); }
inline Vec3 Vec3::operator*(float t) { return Vec3(x * t, y * t, z * t); }
inline Vec3 Vec3::operator/(float t) { return Vec3(x / t, y / t, z / t); }
inline Vec3 operator*(float t,Vec3 v) { return v * t; }
inline Vec3 operator/(float t,Vec3 v) { return v / t; }
inline Vec3& Vec3::operator+=(Vec3 v) { x += v.x; y += v.y; z += v.z; return *this; }
inline Vec3& Vec3::operator-=(Vec3 v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
inline Vec3& Vec3::operator*=(Vec3 v) { x *= v.x; y *= v.y; z *= v.z; return *this; }
inline Vec3& Vec3::operator/=(Vec3 v) { x /= v.x; y /= v.y; z /= v.z; return *this; }
inline Vec3& Vec3::operator*=(float t) { x *= t; y *= t; z *= t; return *this; }
inline Vec3& Vec3::operator/=(float t) { x /= t; y /= t; z /= t; return *this; }

inline bool Vec3::operator==(Vec3 v) { return x == v.x && y == v.y && z == v.z; }
inline bool Vec3::operator!=(Vec3 v) { return !(*this == v); }

inline float Vec3::operator%(Vec3 v) { return x * v.x + y * v.y + z * v.z; }
inline Vec3 Vec3::operator^(Vec3 v) {
  return Vec3(
    y * v.z - z * v.y,
    z * v.x - x * v.z,
    x * v.y - y * v.x
  );
}

inline float Vec3::l() { return std::sqrt(l2()); }
inline float Vec3::l2() { return x * x + y * y + z * z; }
inline Vec3 Vec3::unit() { 
  float len = l(); 
  [[unlikely]] if (!len) {
    printf(
      "runtime warning: [%s:%d] try to get unit vector of a zero vector\n", 
      __FILE__, __LINE__
    );
    return Vec3::i_e();
  }
  return Vec3(x / len, y / len, z / len); 
}
inline float Vec3::dis(Vec3 v1, Vec3 v2) { return (v2 - v1).l(); }

inline Vec3 Vec3::proj_to(Vec3 to) { 
  [[unlikely]] if (!to.l2()) {
    printf(
      "runtime warning: [%s:%d] try to project to a zero vector\n", 
      __FILE__, __LINE__
    );
    return *this;
  }
  return *this % to / to.l2() * to; 
}
inline float Vec3::cos_angle_of(Vec3 v1, Vec3 v2) { return v1 % v2 / std::sqrt(v1.l2() * v2.l2()); }
inline float Vec3::angle(Vec3 v1, Vec3 v2) { return acos(cos_angle_of(v1, v2)); }

inline Vec3 Vec3::reflect(Vec3 normal) {
  if (*this % normal < 0) { normal = - normal; }
  return - *this + 2 * this->proj_to(normal);
}

inline Vec3 Vec3::abs() { return Vec3(std::abs(x), std::abs(y), std::abs(z)); }
inline Vec3 Vec3::floor() { return Vec3(std::floor(x), std::floor(y), std::floor(z)); }
inline Vec3 Vec3::ceil() { return Vec3(std::ceil(x), std::ceil(y), std::ceil(z)); }

inline Vec3 Vec3::random_unit_vec() {
  float u = static_cast<float>(rand()) / RAND_MAX 
          + static_cast<float>(rand()) / RAND_MAX / RAND_MAX;
  float v = static_cast<float>(rand()) / RAND_MAX
          + static_cast<float>(rand()) / RAND_MAX / RAND_MAX;

  float theta = 2.0f * constant::math::pi * u;
  float phi = acos(2.0f * v - 1.0f);

  float x = sin(phi) * cos(theta);
  float y = sin(phi) * sin(theta);
  float z = cos(phi);

  return Vec3(x, y, z);
}

inline int Vec3::R() { return std::min(std::max(int(r * 255.99), 0), 255); }
inline int Vec3::G() { return std::min(std::max(int(g * 255.99), 0), 255); }
inline int Vec3::B() { return std::min(std::max(int(b * 255.99), 0), 255); }

inline Vec3 Vec3::rgb_to_hsv(Vec3 rgb) {
  float r = rgb.r;
  float g = rgb.g;
  float b = rgb.b;

  float max = std::max(r, std::max(g, b));
  float min = std::min(r, std::min(g, b));
  float delta = max - min;

  float h = 0.0f;
  float s = 0.0f;
  float v = max;

  if (delta != 0.0f) {
    s = delta / max;

    if (max == r) { h = (g - b) / delta; } 
    else if (max == g) { h = 2.0f + (b - r) / delta; } 
    else { h = 4.0f + (r - g) / delta; }

    h *= 60.0f;
    if (h < 0.0f) { h += 360.0f; }
  }

  return Vec3(h, s, v);
}

inline Vec3 Vec3::hsv_to_rgb(Vec3 hsv) {
  float h = hsv.x;
  float s = hsv.y;
  float v = hsv.z;

  if (s == 0.0f) {
      return Vec3(v, v, v);
  }

  h /= 60.0f;
  int i = static_cast<int>(h);
  float f = h - i;
  float p = v * (1.0f - s);
  float q = v * (1.0f - s * f);
  float t = v * (1.0f - s * (1.0f - f));

  switch (i) {
    case 0: return Vec3(v, t, p);
    case 1: return Vec3(q, v, p);
    case 2: return Vec3(p, v, t);
    case 3: return Vec3(p, q, v);
    case 4: return Vec3(t, p, v);
    case 5: return Vec3(v, p, q);
    default: return Vec3(v, v, v);
  }
}

inline std::ostream& operator<<(std::ostream& out, Vec3 v) {
  out << '(';
  for (int i = 0; i < 3; i++) {
    out << v.e[i] << (i < 2 ? ", " : "");
  } 
  out << ')';
  return out;
}


#endif