#include "include/ppm.hpp"
#include "include/ray.h"
#include "include/schwarz_metric_ray.h"
#include "include/constant.h"

#include "utils/bar.hpp"

#include <cmath>

constexpr float outer_range = 100.;
constexpr int step_limit = 20000000;

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
      
      float cos_alpha = cos_angle_of(-ray.start, pos - ray.start);
      float l = 1.45 - ray.start.l() * sqrt(1 / cos_alpha / cos_alpha - 1) ;

      Vec3 inner_color = l > 0 && f(int(f(l))) - l < 1e-2
        ? Vec3::white() * cos(pos.z / 3)
        : Vec3::black();

      color += alpha * inner_color;
      break;
    }
    if (pos.l2() >= outer_range * outer_range) { 
      Vec3 background_color = Vec3::black();
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
  constexpr float focal_length = 1e-1;
  constexpr float width = 9.6e-2;

  constexpr int height_px = 1080;
  constexpr int width_px = 1920;
  constexpr float cell_length = width / width_px;

  /* stack is too small for allocating */
  static std::array<std::array<Vec3, width_px>, height_px> pixels;

  Bar bar(height_px * width_px);

  constexpr float phi = 0.02;
  constexpr float alpha = 0.2;

  Vec3 camera = 13 * Vec3(cos(phi), 0, sin(phi));
  Vec3 up_world(0, -sin(alpha), cos(alpha));
  Vec3 forward = (Vec3::origin() - camera).unit();
  Vec3 camera_y = (up_world ^ forward).unit();
  Vec3 camera_x = (camera_y ^ forward).unit();

  for (int i = 0; i < height_px; i++) {
    for (int j = 0; j < width_px; j++) {

      constexpr int TAA_times = 5;

      pixels[i][j] = Vec3::zero();
      for (int _ = 0; _ < 5; _++) {
        Vec3 start = camera;
        Vec3 end = camera + focal_length * (Vec3::origin() - camera).unit()
                + camera_x * (-height_px / 2. + i + float(rand()) / RAND_MAX) * cell_length
                + camera_y * (-width_px / 2. + j + float(rand()) / RAND_MAX) * cell_length;
        SM_Ray ray(start, end - start, Vec3::origin());
        pixels[i][j] += get_color_of_ray_naive_disk(ray);
      }

      pixels[i][j] /= TAA_times;

      bar.step();
    }
  }

  output_ppm("test.ppm", pixels);
}