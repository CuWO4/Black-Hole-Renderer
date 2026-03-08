#include "include/constant.h"
#include "include/camera.h"
#include "include/bloom.hpp"

#include "include/scenes/naive_disk_scene.hpp"
#include "include/render/light_cast_tiled_mt.hpp"

#include "utils/image.hpp"

int main(int argc, char** argv) {
  using namespace constant;

  static Image<image::height_px, image::width_px> image;
  static Image<image::height_px, image::width_px> buffer;

  using namespace camera0;

  Camera camera(
    focal_length, width,
    image::height_px, image::width_px,
    camera_r * Vec3(std::cos(phi), 0, std::sin(phi)),
    Vec3(
      std::cos(elevation) * std::cos(theta),
      std::cos(elevation) * std::sin(theta),
      std::sin(elevation)
    ),
    alpha
  );

  NaiveDiskScene scene("assets/eso0932a.jpg");

  RenderSettings rs;
  rs.taa_times = 5;
  rs.tile_h = 50;
  rs.tile_w = 50;
  rs.thread_n = 0; // auto
  rs.seed = 1;

  light_cast_render_tiled_mt(image, camera, scene, rs);

  renderer_bloom(
    buffer, image,
    std::array<BloomConfig, 3>{
      BloomConfig(0.4, 1 + 5 * (image::width_px / 1920.0)),
      BloomConfig(0.4, 1 + 9 * (image::width_px / 960.0)),
      BloomConfig(0.6, 1 + 17 * (image::width_px / 480.0)),
    }
  );

  image = buffer;
  image.save_as_jpg("test.jpg");
}