#include "include/camera.h"

Camera::Camera(
  float focal_length,
  float width,
  
  int height_px,
  int width_px,

  Vec3 camera_pos,

  /* zero by default representing pointing to the origin */
  Vec3 view_direction,
  /* angle of rotation along view direction, anticlockwise as positive */
  float alpha
): 
  focal_length(focal_length), height_px(height_px), width_px(width_px), 
  cell_length(width / width_px), camera(camera_pos)
{
  Vec3 up_world(0, -sin(alpha), cos(alpha));
  this->view_direction = (Vec3::origin() - camera).unit() + view_direction.unit() - Vec3::i_e();
  camera_y = (up_world ^ this->view_direction).unit();
  camera_x = (camera_y ^ this->view_direction).unit();
}

Vec3 Camera::get_direction_of_i_j(int i, int j) {
  Vec3 start = camera;
  Vec3 end = camera + focal_length * view_direction
            + camera_x * (-height_px / 2. + i + float(rand()) / static_cast<float>(RAND_MAX)) * cell_length
            + camera_y * (-width_px / 2. + j + float(rand()) / static_cast<float>(RAND_MAX)) * cell_length;
  return (end - start).unit();
}