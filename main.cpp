#include "include/constant.h"
#include "include/camera.h"
#include "include/bloom.hpp"

#include "include/scenes/naive_disk_scene.hpp"
#include "include/render/light_cast_tiled_mt.hpp"

#include "utils/image.hpp"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#ifdef _WIN32
#define VIDEO_POPEN _popen
#define VIDEO_PCLOSE _pclose
#else
#define VIDEO_POPEN popen
#define VIDEO_PCLOSE pclose
#endif

static std::string shell_quote(const std::string& raw) {
  std::string quoted;
  quoted.reserve(raw.size() + 2);
  quoted.push_back('\'');
  for (char c : raw) {
    if (c == '\'') {
      quoted += "'\\''";
    } else {
      quoted.push_back(c);
    }
  }
  quoted.push_back('\'');
  return quoted;
}

static void print_usage(const char* argv0) {
  std::cerr << "usage: " << argv0 << " -o target.mp4\n";
}

static FILE* open_video_pipe(const std::string& out_path) {
  using namespace constant;

  std::ostringstream command;
  command
    << "ffmpeg -y -f rawvideo -pix_fmt rgb24 -video_size "
    << video::width_px << 'x' << video::height_px
    << " -framerate " << video::fps
    << " -i - -an -c:v libx265 -preset slow -crf " << video::crf
    << " -tag:v hvc1 -movflags +faststart"
    << " -pix_fmt yuv420p "
    << shell_quote(out_path);

  return VIDEO_POPEN(command.str().c_str(), "w");
}

static std::string format_eta_seconds(double seconds_total) {
  const long long seconds = static_cast<long long>(seconds_total + 0.5);
  const long long hours = seconds / 3600;
  const long long minutes = (seconds % 3600) / 60;
  const long long secs = seconds % 60;

  std::ostringstream out;
  out << std::setfill('0')
      << std::setw(2) << hours << ':'
      << std::setw(2) << minutes << ':'
      << std::setw(2) << secs;
  return out.str();
}

int main(int argc, char** argv) {
  std::string out_path;

  for (int i = 1; i < argc; i++) {
    const std::string arg = argv[i];

    if (arg == "-o" || arg == "--out") {
      if (i + 1 >= argc) {
        std::cerr << "error: " << arg << " requires a value\n";
        print_usage(argv[0]);
        return 1;
      }
      out_path = argv[++i];
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

  if (out_path.empty()) {
    std::cerr << "error: missing -o target.mp4\n";
    print_usage(argv[0]);
    return 1;
  }

  using namespace constant;

  static Image<video::height_px, video::width_px> image;
  static Image<video::height_px, video::width_px> buffer;

  NaiveDiskScene scene("assets/eso0932a.jpg");

  RenderSettings rs;
  rs.taa_times = 2;
  rs.tile_h = 50;
  rs.tile_w = 50;
  rs.thread_n = 0; // auto
  rs.seed = 1;

  FILE* video_pipe = open_video_pipe(out_path);
  if (!video_pipe) {
    std::cerr << "error: failed to start ffmpeg\n";
    return 1;
  }

  const float orbit_radius_xy = camera0::camera_r * std::cos(camera0::phi);
  const float orbit_z = camera0::camera_r * std::sin(camera0::phi);
  const auto render_start = std::chrono::steady_clock::now();

  for (int frame_index = 0; frame_index < video::frame_count; frame_index++) {
    const float orbit_phase = (2.0f * math::pi * frame_index) / video::frame_count;

    Camera camera(
      camera0::focal_length,
      camera0::width,
      video::height_px,
      video::width_px,
      Vec3(
        orbit_radius_xy * std::cos(orbit_phase),
        orbit_radius_xy * std::sin(orbit_phase),
        orbit_z
      ),
      Vec3::i_e(),
      camera0::alpha
    );

    std::cout << "rendering frame " << (frame_index + 1) << '/' << video::frame_count << std::endl;
    light_cast_render_tiled_mt(image, camera, scene, rs);

    renderer_bloom(
      buffer, image,
      std::array<BloomConfig, 2>{
        BloomConfig(0.4, 1 + 5 * (video::width_px / 1920.0)),
        BloomConfig(0.4, 1 + 9 * (video::width_px / 960.0))
      }
    );

    image = buffer;

    const auto rgb = image.to_rgb_buffer();
    const size_t expected_bytes = rgb.size();
    const size_t written_bytes = fwrite(rgb.data(), 1, expected_bytes, video_pipe);
    if (written_bytes != expected_bytes) {
      std::cerr << "error: failed to write frame " << frame_index << " to ffmpeg\n";
      VIDEO_PCLOSE(video_pipe);
      return 1;
    }

    const int frames_done = frame_index + 1;
    const auto now = std::chrono::steady_clock::now();
    const double elapsed_seconds = std::chrono::duration<double>(now - render_start).count();
    const double seconds_per_frame = elapsed_seconds / frames_done;
    const double eta_seconds = seconds_per_frame * (video::frame_count - frames_done);

    std::cout
      << "frame " << frames_done << '/' << video::frame_count
      << " done, eta " << format_eta_seconds(eta_seconds)
      << std::endl;
  }

  if (VIDEO_PCLOSE(video_pipe) != 0) {
    std::cerr << "error: ffmpeg exited with failure\n";
    return 1;
  }

  std::cout << "saved to " << out_path << std::endl;
  return 0;
}