#ifndef NAIVE_DISK_SCENE_H__
#define NAIVE_DISK_SCENE_H__

#include "../ray.h"
#include "../background.h"
#include "../berlin_noise.h"
#include "../constant.h"
#include "../color.h"
#include "../utils/rng.hpp"

#include <cmath>

class NaiveDiskScene {
public:
  explicit NaiveDiskScene(const std::string& background_path)
    : star_sky(background_path),
      shape_noise(
        constant::model::shape_noise_detail_level,
        constant::model::noise_frequency0,
        constant::model::shape_noise_detail_coef,
        constant::model::shape_noise_superposition_intensity,
        constant::model::shape_noise_contrast
      ),
      cloud_noise(
        constant::model::cloud_noise_detail_level,
        constant::model::noise_frequency0,
        constant::model::shape_noise_detail_coef,
        constant::model::shape_noise_superposition_intensity,
        constant::model::shape_noise_contrast
      ) {}

  float get_dl(Vec3 pos) const {
    return constant::renderer::dl0 * std::min(
      pos.l(),
      std::abs(pos.z) / constant::model::disk_thickness / 1.2f + 0.1f
    );
  }

  Vec3 operator()(Ray& ray, XorShift32Rng& rng) {
    Vec3 color = Vec3::black();
    float alpha = 1.0f;

    ray.step(get_dl(ray.get_position()) * rng.next_f01());

    for (int step_n = 0; step_n < constant::renderer::step_limit; step_n++) {
      Vec3 pos = ray.get_position();

      if (pos.l2() < 0.9f * 0.9f) {
        color += alpha * Vec3::black();
        break;
      }

      if (pos.l2() >= constant::renderer::outer_range * constant::renderer::outer_range) {
        Vec3 background_color = star_sky.sample_equirect(pos);

#ifdef RED_BLUE_BACKGROUND
        float t1 = std::atan(pos.y / pos.x);
        float t2 = std::asin(pos.z / pos.l());
        background_color = (int(t1 / 0.01f) + int(t2 / 0.01f)) % 2
          ? Vec3::blue()
          : Vec3::red();
#endif

        color += alpha * background_color;
        break;
      }

      float r = std::sqrt(pos.x * pos.x + pos.y * pos.y);
      float dl = get_dl(ray.get_position());

      if (
        std::abs(pos.z) < constant::model::disk_thickness / 2.0f &&
        r < constant::model::Rout && r > constant::model::Rin
      ) {
        float thickness = constant::model::disk_thickness
          * (0.6f + 0.4f * shape_noise.get_noise(Vec3(pos.x, pos.y, 0)));
        float thickness_factor = std::min(1.0f, 1.0f - std::abs(pos.z / thickness));

        float l = std::min(constant::model::Rout - r, r - constant::model::Rin);
        float edge_factor = 1.0f - 1.0f / (2.5f * l + 1.0f);

        float density = 0.7f * edge_factor * thickness_factor * cloud_noise.get_noise(pos);
        if (density < 0) density = 0;

        float temperature = color::get_disk_temperature(r * constant::model::Rs_m);
        temperature = color::maximum_temperature() - 2.0f * (color::maximum_temperature() - temperature);

        Vec3 disk_color = color::black_body_color(temperature);

        auto f = [](float x) { return x; };
        float brightness = f(1.5f * temperature / color::maximum_temperature());

        float tau = density * constant::model::disk_opacity * dl;
        float T = std::exp(-tau);

        Vec3 S = constant::model::disk_luminous_intensity * brightness * disk_color;

        color += alpha * (1.0f - T) * S;
        alpha *= T;

        if (alpha < 1e-4f) break;
      }

      ray.step(dl);
    }

    return color;
  }

private:
  Background star_sky;
  BerlinNoise shape_noise;
  BerlinNoise cloud_noise;
};

#endif