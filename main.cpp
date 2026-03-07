#include "include/ray.h"
#include "include/constant.h"
#include "include/camera.h"
#include "include/light_cast.hpp"
#include "include/berlin_noise.h"
#include "include/color.h"
#include "include/bloom.hpp"
#include "include/background.h"

#include "utils/image.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "lib/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "lib/stb_image_write.h"

#include <cmath>

Background star_sky("assets/eso0932a.jpg");

float get_dl(Vec3 pos) {
  return constant::renderer::dl0 * std::min(
    pos.l(),
    abs(pos.z) / constant::model::disk_thickness / 1.2f + 0.1f
  );
}

Vec3 get_color_of_ray_naive_disk(Ray& ray) {
  static BerlinNoise shape_noise(
    constant::model::shape_noise_detail_level,
    constant::model::noise_frequency0,
    constant::model::shape_noise_detail_coef,
    constant::model::shape_noise_superposition_intensity,
    constant::model::shape_noise_contrast
  );
  static BerlinNoise cloud_noise(
    constant::model::cloud_noise_detail_level,
    constant::model::noise_frequency0,
    constant::model::shape_noise_detail_coef,
    constant::model::shape_noise_superposition_intensity,
    constant::model::shape_noise_contrast
  );

  Vec3 color = Vec3::black();
  float alpha = 1;

  ray.step(get_dl(ray.get_position()) * rand() / static_cast<float>(RAND_MAX));

  for(
    int step_n = 0;
    step_n < constant::renderer::step_limit;
    step_n++
  ) {
    Vec3 pos = ray.get_position();
    if (pos.l2() < 1.5 * 1.5) {
      color += alpha * Vec3::black();
      break;
    }
    if (pos.l2() >= constant::renderer::outer_range * constant::renderer::outer_range) {
      // Vec3 background_color = Vec3::black();
      Vec3 background_color = star_sky.sample_equirect(pos);

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
      temperature = color::maximum_temperature() - 2 * (color::maximum_temperature() - temperature);
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
  static Image<image::height_px, image::width_px> buffer;

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

  renderer_bloom(
    buffer, image,
    std::array<BloomConfig, 3> {
      BloomConfig(0.4, 1 + 5 * (image::width_px / 1920.)),
      BloomConfig(0.4, 1 + 9 * (image::width_px / 960.)),
      BloomConfig(0.6, 1 + 17 * (image::width_px / 480.)),
    }
  );
  image = buffer;

  image.save_as_jpg("test.jpg");
}