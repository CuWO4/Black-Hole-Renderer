#include "include/ray.h"
#include "include/constant.h"
#include "include/camera.h"
#include "include/light_cast.hpp"

#include "utils/image.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "lib/stb_image_write.h"

#include <cmath>

float get_dl(Vec3 pos) {
  return 0.03 * pos.l();
}

Vec3 get_color_of_ray_naive_disk(Ray& ray) {
  Vec3 color = Vec3::black();
  float alpha = 1;

  ray.step(get_dl(ray.get_position()) * rand() / RAND_MAX);
  
  int step_n;
  for(
    step_n = 0;
    step_n < constant::renderer::step_limit;
    step_n++
  ) {
    Vec3 pos = ray.get_position();
    if (pos.l2() < 1.5 * 1.5) {
      auto f = [](float x) { return 1 / (15 * x); };
      
      float cos_alpha = Vec3::cos_angle_of(-ray.start, pos - ray.start);
      float l = 1.45 - ray.start.l() * sqrt(1 / cos_alpha / cos_alpha - 1) ;

      Vec3 inner_color = l > 0 && f(int(f(l))) - l < 1e-2
        ? Vec3::white() * cos(pos.z / 3)
        : Vec3::black();

      color += alpha * inner_color;
      break;
    }
    if (pos.l2() >= constant::renderer::outer_range * constant::renderer::outer_range) { 
      Vec3 background_color = Vec3::black();

      float t1 = atan(pos.y / pos.x);
      float t2 = asin(pos.z / pos.l());
      background_color = (int(t1 / 0.01) + int(t2 / 0.01)) % 2
        ? Vec3::blue()
        : Vec3::red();

      color += alpha * background_color;
      break;
    }

    float r = sqrt(pos.x * pos.x + pos.y * pos.y);
    float dl = get_dl(ray.get_position());
    if (
      abs(pos.z) < constant::model::disk_thickness / 2.
      && r < constant::model::Rout && r > constant::model::Rin
    ) {
      color += alpha * 0.75 * Vec3::white() * dl;
      alpha *= 1 - 0.5 * dl;
    }
    ray.step(dl);
  }

  return color;
}


int main(int argc, char** argv) {

  using namespace constant;

  /* stack is too small for allocating */
  static Image<image::height_px, image::width_px> image;

  using namespace camera0;

  Camera camera(
    focal_length, width, 
    image::height_px,  image::width_px,
    camera_r * Vec3(cos(phi), 0, sin(phi)),
    Vec3(
      cos(elevation) * cos(theta), 
      cos(elevation) * sin(theta), 
      sin(elevation)
    ), alpha
  );

  light_cast_render<image::height_px, image::width_px>(
    image,
    camera,
    get_color_of_ray_naive_disk
  );

  image.save_as_jpg("test.jpg");
}