#include "include/ppm.hpp"
#include "include/ray.h"
#include "include/schwarz_metric_ray.h"
#include "include/constant.h"

#include "utils/bar.hpp"

#include <cmath>

constexpr float outer_range = 100.;
constexpr int step_limit = 20000000;

float get_dl(Vec3 pos) {
  return 0.05 * pos.l();
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
    if (pos.l2() <= 1. || pos.l2() >= outer_range * outer_range) { 
      color += alpha * Vec3::black();
      break;
    }

    float r = sqrt(pos.x * pos.x + pos.y * pos.y);
    float dl = get_dl(ray.get_position());
    if (
      abs(pos.z) < CONSTANT_disk_thickness / 2.
      && r < CONSTANT_Rout && r > CONSTANT_Rin
    ) {
      color += alpha * 0.15 * Vec3::white() * dl;
      alpha *= 1 - 0.05 * dl;
    }
    ray.step(dl);
  }

  return color;
}



int main(int argc, char** argv) {
  constexpr float focal_length = 1e-3;
  constexpr float width = 9.6e-4;

  constexpr int height_px = 400;
  constexpr int width_px = 600;
  constexpr float cell_length = width / width_px;

  /* stack is too small for allocating */
  static std::array<std::array<Vec3, width_px>, height_px> pixels;

  Bar bar(height_px * width_px);

  constexpr float phi = 0.1;

  Vec3 camera = 20 * Vec3(cos(phi), 0, sin(phi));
  Vec3 forward = (Vec3::origin() - camera).unit();
  Vec3 up_world(0, 0, 1);
  Vec3 camera_y = (up_world ^ forward).unit();
  Vec3 camera_x = (camera_y ^ forward).unit();

  for (int i = 0; i < height_px; i++) {
    for (int j = 0; j < width_px; j++) {
      Vec3 start = camera;
      Vec3 end = camera + focal_length * (Vec3::origin() - camera).unit()
               + camera_x * (-height_px / 2. + i) * cell_length
               + camera_y * (-width_px / 2. + j) * cell_length;
      SM_Ray ray(start, end - start, Vec3::origin());
      pixels[i][j] = get_color_of_ray_naive_disk(ray);
      bar.step();
    }
  }

  output_ppm("test.ppm", pixels);
}