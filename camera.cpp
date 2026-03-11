#include "include/camera.h"

#include <cmath>

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
  this->view_direction = (Vec3::origin() - camera).unit() + view_direction.unit() - Vec3::i_e();

  Vec3 up_world = Vec3::k_e();
  if (std::abs(this->view_direction % up_world) > 0.999f) {
    up_world = Vec3::j_e();
  }

  Vec3 right = (up_world ^ this->view_direction).unit();
  Vec3 up = (this->view_direction ^ right).unit();

  const float sin_alpha = std::sin(alpha);
  const float cos_alpha = std::cos(alpha);

  camera_y = (cos_alpha * right + sin_alpha * up).unit();
  camera_x = (cos_alpha * up - sin_alpha * right).unit();
}

Vec3 Camera::get_direction_of_i_j(int i, int j) {
  Vec3 start = camera;
  Vec3 end = camera + focal_length * view_direction
            + camera_x * (-height_px / 2. + i + float(rand()) / static_cast<float>(RAND_MAX)) * cell_length
            + camera_y * (-width_px / 2. + j + float(rand()) / static_cast<float>(RAND_MAX)) * cell_length;
  return (end - start).unit();
}