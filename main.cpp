#include "include/ray.h"
#include "include/constant.h"
#include "include/camera.h"
#include "include/light_cast.hpp"

#include "utils/image.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "lib/stb_image_write.h"

#include <cmath>

constexpr float outer_range = 100.;
constexpr int step_limit = 20000;

constexpr float focal_length = 8.5e-2;
constexpr float width = 9.6e-2;
constexpr float camera_r = 13;
constexpr float phi = 0.02;
constexpr float alpha = 0.2;

constexpr int height_px = 200;
constexpr int width_px = 300;

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
    step_n < step_limit;
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
    if (pos.l2() >= outer_range * outer_range) { 
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
      abs(pos.z) < Constant::disk_thickness / 2.
      && r < Constant::Rout && r > Constant::Rin
    ) {
      color += alpha * 0.75 * Vec3::white() * dl;
      alpha *= 1 - 0.5 * dl;
    }
    ray.step(dl);
  }

  return color;
}


int main(int argc, char** argv) {

  /* stack is too small for allocating */
  static Image<height_px, width_px> image;

  Camera camera(focal_length, width, height_px, width_px, camera_r, phi, alpha);

  light_cast_render<height_px, width_px>(
    image,
    camera,
    get_color_of_ray_naive_disk
  );

  image.save_as_jpg("test.jpg");
}