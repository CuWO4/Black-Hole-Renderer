#include "include/ray.h"
#include "include/constant.h"
#include "include/camera.h"
#include "include/light_cast.hpp"
#include "include/berlin_noise.h"
#include "include/color.h"

#include "utils/image.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "lib/stb_image_write.h"

#include <cmath>

float get_dl(Vec3 pos) {
  return constant::renderer::dl0 * std::min(
    pos.l(),
    abs(pos.z) / constant::model::disk_thickness / 1.2f + 0.1f
  );
}

Vec3 get_color_of_ray_naive_disk(Ray& ray) {
  BerlinNoise shape_noise(
    constant::model::shape_noise_detail_level, 
    constant::model::noise_frequency0,
    constant::model::shape_noise_detail_coef,
    constant::model::shape_noise_superposition_intensity,
    constant::model::shape_noise_contrast
  );
  BerlinNoise cloud_noise(
    constant::model::cloud_noise_detail_level, 
    constant::model::noise_frequency0,
    constant::model::shape_noise_detail_coef,
    constant::model::shape_noise_superposition_intensity,
    constant::model::shape_noise_contrast
  );
  
  Vec3 color = Vec3::black();
  float alpha = 1;

  ray.step(get_dl(ray.get_position()) * rand() / RAND_MAX);
  
  for(
    int step_n = 0;
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

      #ifdef RED_BLUE_BACKGROUND
      float t1 = atan(pos.y / pos.x);
      float t2 = asin(pos.z / pos.l());
      background_color = (int(t1 / 0.01) + int(t2 / 0.01)) % 2
        ? Vec3::blue()
        : Vec3::red();
      #endif

      color += alpha * background_color;
      break;
    }

    float r = sqrt(pos.x * pos.x + pos.y * pos.y);
    float dl = get_dl(ray.get_position());
    if (
      abs(pos.z) < constant::model::disk_thickness / 2.
      && r < constant::model::Rout && r > constant::model::Rin
    ) {
      float thickness = constant::model::disk_thickness * (0.6 + 0.4 * shape_noise.get_noise(Vec3(pos.x, pos.y, 0)));
      float thickness_factor = std::min(1.f, 1 - abs(pos.z / thickness));

      float l = std::min(constant::model::Rout - r, r - constant::model::Rin);
      float edge_factor = 1 - 1 / (2.5 * l + 1);

      float density = 0.7 * edge_factor * thickness_factor * cloud_noise.get_noise(pos);
      if (density < 0) density = 0;

      float temperature = color::get_disk_temperature(r * constant::model::Rs_m);
      temperature = color::maximum_temperature() - 2.25 * (color::maximum_temperature() - temperature);
      Vec3 disk_color = color::black_body_color(temperature);

      auto f = [](float x) { return x; };
      float brightness = f(1.5 * temperature / color::maximum_temperature());

      color += density * brightness * alpha * constant::model::disk_luminous_intensity * disk_color * dl;
      alpha *= 1 - density * constant::model::disk_opacity * dl;
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