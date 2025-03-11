#include "include/camera.h"

Camera::Camera(
  float focal_length,
  float width,
  int height_px,
  int width_px,
  float camera_r, /* length from origin */
  float phi,      /* angle of upward rotation */
  float alpha     /* angle of rotation along the axis */
): focal_length(focal_length), height_px(height_px), width_px(width_px), cell_length(width / width_px)  {
  camera = camera_r * Vec3(cos(phi), 0, sin(phi));
  Vec3 up_world(0, -sin(alpha), cos(alpha));
  Vec3 forward = (Vec3::origin() - camera).unit();
  camera_y = (up_world ^ forward).unit();
  camera_x = (camera_y ^ forward).unit();
}

Vec3 Camera::get_direction_of_i_j(int i, int j) {
  Vec3 start = camera;
  Vec3 end = camera + focal_length * (Vec3::origin() - camera).unit()
            + camera_x * (-height_px / 2. + i + float(rand()) / RAND_MAX) * cell_length
            + camera_y * (-width_px / 2. + j + float(rand()) / RAND_MAX) * cell_length;
  return (end - start).unit();
}