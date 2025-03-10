#include "include/vec3.h"

#include <cmath>
#include <algorithm>

Vec3::Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

Vec3& Vec3::operator+() {
  return *this;
}

Vec3 Vec3::operator-() {
  return Vec3(-x, -y, -z);
}

float Vec3::operator[](int i) const {
  return e[i];
}

float& Vec3::operator[](int i) {
  return e[i];
}

Vec3 Vec3::operator+(Vec3 v) {
  return Vec3(x + v.x, y + v.y, z + v.z);
}

Vec3 Vec3::operator-(Vec3 v) {
  return Vec3(x - v.x, y - v.y, z - v.z);
}

Vec3 Vec3::operator*(Vec3 v) {
  return Vec3(x * v.x, y * v.y, z * v.z);
}

Vec3 Vec3::operator/(Vec3 v) {
  return Vec3(x / v.x, y / v.y, z / v.z);
}

Vec3 Vec3::operator*(float t) {
  return Vec3(x * t, y * t, z * t);
}

Vec3 Vec3::operator/(float t) {
  return Vec3(x / t, y / t, z / t);
}

Vec3 operator*(float t, Vec3 v) {
  return v * t;
}

Vec3 operator/(float t, Vec3 v) {
  return v / t;
}

Vec3& Vec3::operator+=(Vec3 v) {
  x += v.x;
  y += v.y;
  z += v.z;
  return *this;
}

Vec3& Vec3::operator-=(Vec3 v) {
  x -= v.x;
  y -= v.y;
  z -= v.z;
  return *this;
}

Vec3& Vec3::operator*=(Vec3 v) {
  x *= v.x;
  y *= v.y;
  z *= v.z;
  return *this;
}

Vec3& Vec3::operator/=(Vec3 v) {
  x /= v.x;
  y /= v.y;
  z /= v.z;
  return *this;
}

Vec3& Vec3::operator*=(float t) {
  x *= t;
  y *= t;
  z *= t;
  return *this;
}

Vec3& Vec3::operator/=(float t) {
  x /= t;
  y /= t;
  z /= t;
  return *this;
}

float Vec3::operator%(Vec3 v) {
  return x * v.x + y * v.y + z * v.z;
}

Vec3 Vec3::operator^(Vec3 v) {
  return Vec3(
    y * v.z - z * v.y,
    z * v.x - x * v.z,
    x * v.y - y * v.x
  );
}

float Vec3::l() {
  return std::sqrt(l2());
}

float Vec3::l2() {
  return x * x + y * y + z * z;
}

Vec3 Vec3::unit() {
  float len = l();
  return Vec3(x / len, y / len, z / len);
}

int Vec3::R() {
  return std::min(std::max(int(r * 255.99), 0), 255);
}

int Vec3::G() {
  return std::min(std::max(int(g * 255.99), 0), 255);
}

int Vec3::B() {
  return std::min(std::max(int(b * 255.99), 0), 255);
}

std::ostream& operator<<(std::ostream& out, Vec3& v) {
  out << '(';
  for (int i = 0; i < 3; i++) {
    out << v.e[i] << (i < 2 ? ", " : "");
  } 
  out << ')';
  return out;
}
