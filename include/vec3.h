#ifndef VEC3_H__
#define VEC3_H__

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
  friend Vec3 operator*(float t, Vec3 v);
  friend Vec3 operator/(float t, Vec3 v);
  Vec3& operator+=(Vec3 v);
  Vec3& operator-=(Vec3 v);
  Vec3& operator*=(Vec3 v);
  Vec3& operator/=(Vec3 v);
  Vec3& operator*=(float t);
  Vec3& operator/=(float t);

  float operator%(Vec3 v); /* dot product   */
  Vec3 operator^(Vec3 v);  /* cross product */

  float l();
  float l2();
  Vec3 unit();

public:
  union {
    struct { float x, y, z; };
    struct { float r, g, b; }; /* [0, 1] */
    float e[3];
  };

  int R(); /* [0, 255] */
  int G(); /* [0, 255] */
  int B(); /* [0, 255] */

public:
  friend std::ostream& operator<<(std::ostream& out, Vec3& v);
};

const Vec3 ORIGIN = Vec3(0, 0, 0);
const Vec3 i_e = Vec3(1, 0, 0);
const Vec3 j_e = Vec3(0, 1, 0);
const Vec3 k_e = Vec3(0, 0, 1);

const Vec3 BLACK = Vec3(0, 0, 0);
const Vec3 RED = Vec3(1, 0, 0);
const Vec3 GREEN = Vec3(0, 1, 0);
const Vec3 BLUE = Vec3(0, 0, 1);
const Vec3 WHITE = Vec3(1, 1, 1);

#endif