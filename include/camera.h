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
    Vec3 camera_pos,
    /* i_e by default representing pointing to the origin */
    Vec3 view_direction = Vec3::i_e(),
    /* angle of rotation along view direction, anticlockwise as positive */
    float alpha = 0.f
  );

  Vec3 get_direction_of_i_j(int i, int j);

  Vec3 get_camera() { return camera; }
  Vec3 get_camera_y() { return camera_y; }
  Vec3 get_camera_x() { return camera_x; }
  Vec3 get_view_direction() { return view_direction; }

private:
  float focal_length;
  int height_px, width_px;
  float cell_length;
  Vec3 camera, camera_y, camera_x, view_direction;
};

#endif