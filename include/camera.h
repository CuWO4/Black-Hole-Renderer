#ifndef CAMERA_H__
#define CAMERA_H__

#include "vec3.hpp"

class Camera {
public:
  Camera(
    float focal_length,
    float width,
    int height_px,
    int width_px,
    float camera_r,   /* length from origin */
    float phi = 0.f,  /* angle of upward rotation */
    float alpha = 0.f /* angle of rotation along the axis */
  );

  Vec3 get_direction_of_i_j(int i, int j);

  Vec3 get_camera() { return camera; }
  Vec3 get_camera_y() { return camera_y; }
  Vec3 get_camera_x() { return camera_x; }

private:
  float focal_length;
  int height_px, width_px;
  float cell_length;
  Vec3 camera, camera_y, camera_x;
};

#endif