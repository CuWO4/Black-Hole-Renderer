#include "include/constant.h"
#include "include/camera.h"
#include "include/bloom.hpp"

#include "include/scenes/naive_disk_scene.hpp"
#include "include/render/light_cast_tiled_mt.hpp"

#include "utils/image.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iostream>
#include <string>

static std::string to_lower_ascii(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
    return (char)std::tolower(c);
  });
  return s;
}

static std::string normalize_ext(std::string ext) {
  ext = to_lower_ascii(ext);
  if (!ext.empty() && ext[0] == '.') ext.erase(ext.begin());
  if (ext == "jpeg") ext = "jpg";
  return ext;
}

static std::string ext_from_path(const std::string& path) {
  const auto slash = path.find_last_of("/\\");
  const auto dot = path.find_last_of('.');
  if (dot == std::string::npos) return "";
  if (slash != std::string::npos && dot < slash) return "";
  return normalize_ext(path.substr(dot + 1));
}

static void print_usage(const char* argv0) {
  std::cerr
    << "usage: " << argv0 << " [--ext png|jpg|bmp|ppm] [-o OUTPUT] [--conf CONF]\n"
    << "  --ext   output format (default: png)\n"
    << "  -o      output file path (default: test.<ext>)\n"
    << "  --conf  reserved (currently ignored)\n";
}

int main(int argc, char** argv) {
  std::string out_ext = "png";
  bool out_ext_explicit = false;
  std::string out_path;

  for (int i = 1; i < argc; i++) {
    const std::string arg = argv[i];

    if (arg == "--ext") {
      if (i + 1 >= argc) {
        std::cerr << "error: --ext requires a value\n";
        print_usage(argv[0]);
        return 1;
      }
      out_ext = normalize_ext(argv[++i]);
      out_ext_explicit = true;
      continue;
    }

    if (arg == "-o" || arg == "--out") {
      if (i + 1 >= argc) {
        std::cerr << "error: " << arg << " requires a value\n";
        print_usage(argv[0]);
        return 1;
      }
      out_path = argv[++i];
      continue;
    }

    if (arg == "--conf") {
      if (i + 1 >= argc) {
        std::cerr << "error: --conf requires a value\n";
        print_usage(argv[0]);
        return 1;
      }
      const std::string conf = argv[++i];
      std::cerr << "warning: --conf is reserved and currently ignored: " << conf << "\n";
      continue;
    }

    if (arg == "-h" || arg == "--help") {
      print_usage(argv[0]);
      return 0;
    }

    std::cerr << "error: unknown argument: " << arg << "\n";
    print_usage(argv[0]);
    return 1;
  }

  if (!out_ext_explicit && !out_path.empty()) {
    const std::string inferred = ext_from_path(out_path);
    if (!inferred.empty()) out_ext = inferred;
  }

  if (out_path.empty()) {
    out_path = std::string("test.") + out_ext;
  }

  if (
    out_ext != "png" &&
    out_ext != "jpg" &&
    out_ext != "bmp" &&
    out_ext != "ppm"
  ) {
    std::cerr << "error: unsupported --ext: " << out_ext << "\n";
    print_usage(argv[0]);
    return 1;
  }

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

  if (out_ext == "png") return image.save_as_png(out_path);
  if (out_ext == "jpg") return image.save_as_jpg(out_path);
  if (out_ext == "bmp") return image.save_as_bmp(out_path);
  return image.save_as_ppm(out_path);
}