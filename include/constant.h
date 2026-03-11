#ifndef CONSTANT_H__
#define CONSTANT_H__

#include <array>
#include <cstdint>

namespace constant {
  namespace math {
    constexpr float pi = 3.14159265358979323846f;
  }

  namespace physics {
    constexpr float G = 6.67430e-11f;
    constexpr float c = 299792458.f;
    constexpr float sigma = 5.670367e-8f;

    constexpr float M_Sun = 1.989e30f /* kg */;
    constexpr float ly = 9454254955488000.f /* m */;
  }

  namespace model {
    constexpr float M = 1.49e7f /* M_SUN */;
    constexpr float M_kg = M * physics::M_Sun;
    constexpr float Rs_m = 2. * physics::G * M * physics::M_Sun / physics::c / physics::c /* m */;
    constexpr float Rs = Rs_m / physics::ly /* ly */;

    constexpr float disk_thickness = 1.5 /* Rs */;
    constexpr float Rin = 2.5 /* Rs */;
    constexpr float Rout = 10 /* Rs */;

    constexpr float disk_luminous_intensity = 1.2;
    constexpr float disk_opacity = 0.65;

    constexpr float shape_noise_detail_coef = 3;
    constexpr float shape_noise_superposition_intensity = 0.1;
    constexpr float shape_noise_contrast = 80;

    constexpr int shape_noise_detail_level = 3;
    constexpr int cloud_noise_detail_level = 7;
    constexpr float noise_frequency0 = 1.15;

    constexpr float temperature_coeff = 1.4;
  }

  namespace renderer {
    constexpr float outer_range = 100.;
    constexpr int step_limit = 20000;
    constexpr float dl0 = 0.15;
  }

  namespace camera0 {
    constexpr float focal_length = 3.3e-2;
    constexpr float width = 9.6e-2;
    constexpr float camera_r = 12;
    constexpr float phi = 0.02;
    constexpr float alpha = 0.2;
  }

  namespace video {
    constexpr int width_px = 1920;
    constexpr int height_px = 1080;
    constexpr int fps = 30;
    constexpr int duration_seconds = 180;
    constexpr int frame_count = fps * duration_seconds;
    constexpr int crf = 14;
  }

  namespace output {
    constexpr int raw_rgb_channels = 3;
    constexpr const char* encoder = "hevc_nvenc";
    constexpr const char* preset = "p7";
    constexpr const char* rate_control = "vbr";
    constexpr const char* pixel_format = "yuv420p";
    constexpr const char* codec_tag = "hvc1";
  }

  namespace cuda_renderer {
    constexpr int frame_batch = 300;
    constexpr int tile_h = 32;
    constexpr int tile_w = 64;
    constexpr int threads = 256;
    constexpr int postprocess_slot_count = 2;
    constexpr int async_video_queue_depth = 16;

    constexpr int shape_noise_w = 1024;
    constexpr int shape_noise_h = 1024;
    constexpr int cloud_noise_x = 256;
    constexpr int cloud_noise_y = 256;
    constexpr int cloud_noise_z = 64;
    constexpr int color_lut_n = 4096;

    constexpr std::uint32_t seed = 1u;
    constexpr unsigned taa_times = 2;

    constexpr float vec_norm_epsilon = 1e-20f;
    constexpr float trace_hit_epsilon = 1e-12f;
    constexpr float background_dir_epsilon = 1e-6f;
    constexpr float capture_radius = 0.9f;
    constexpr float dl_z_scale = 1.2f;
    constexpr float dl_z_bias = 0.1f;
    constexpr float disk_thickness_base = 0.6f;
    constexpr float disk_thickness_shape_scale = 0.4f;
    constexpr float disk_density_scale = 0.7f;
    constexpr float disk_edge_falloff = 2.5f;
    constexpr float alpha_stop_threshold = 1e-4f;
    constexpr float lut_temperature_shift = 2.0f;
    constexpr float lut_brightness_scale = 1.5f;

    constexpr float bloom_highlight_threshold = 1.2f;
    constexpr std::array<float, 3> bloom_superposition_coeffs{0.4f, 0.4f, 0.4f};
    constexpr std::array<float, 3> bloom_radius_scales{5.0f, 9.0f, 13.0f};
    constexpr std::array<float, 3> bloom_reference_widths{1920.0f, 960.0f, 960.0f};
  }
}


#endif