#include "include/render/cuda_video_renderer.hpp"

#include "include/constant.h"
#include "include/berlin_noise.h"
#include "include/color.h"

#include "lib/stb_image.h"

#include <cuda_runtime.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace {

#define CUDA_CHECK(x)                                                         \
  do {                                                                        \
    cudaError_t err__ = (x);                                                  \
    if (err__ != cudaSuccess) {                                               \
      std::cerr << "cuda error: " << cudaGetErrorString(err__)                \
                << " @ " << __FILE__ << ":" << __LINE__ << std::endl;         \
      std::abort();                                                           \
    }                                                                         \
  } while (0)

struct DevVec3 {
  float x, y, z;
};

struct BloomConfig {
  float superposition_coeff;
  int gaussian_kernel_size;
};

struct DeviceFrameCamera {
  DevVec3 camera;
  DevVec3 camera_x;
  DevVec3 camera_y;
  DevVec3 view_direction;
  float focal_length;
  float cell_length;
};

struct DeviceScene {
  float outer_range2;
  int step_limit;
  float dl0;

  float disk_thickness;
  float Rin;
  float Rout;
  float Rs_m;
  float disk_luminous_intensity;
  float disk_opacity;

  uint32_t seed;
  unsigned taa_times;

  int bg_w;
  int bg_h;

  int shape_w;
  int shape_h;

  int cloud_x;
  int cloud_y;
  int cloud_z;

  int color_n;
};

struct CudaAssets {
  bool ready = false;

  unsigned char* d_background = nullptr;
  float* d_shape_noise = nullptr;
  float* d_cloud_noise = nullptr;
  DevVec3* d_color_lut = nullptr;

  DevVec3* d_batch_frames = nullptr;
  int batch_frame_capacity = 0;

  DeviceScene scene{};

  int bg_w = 0;
  int bg_h = 0;
};

CudaAssets g;

struct PostprocessSlot {
  DevVec3* d_frame = nullptr;
  DevVec3* d_highlight = nullptr;
  DevVec3* d_blur_tmp = nullptr;
  DevVec3* d_blur_out = nullptr;
  unsigned char* d_rgb24 = nullptr;
  unsigned char* h_rgb24 = nullptr;
  cudaStream_t stream = nullptr;
  int frame_capacity = 0;
  int rgb24_capacity = 0;
  int pending_frame_index = -1;
};

std::array<PostprocessSlot, constant::cuda_renderer::postprocess_slot_count> g_post_slots{};

struct QueuedRgbFrame {
  std::vector<unsigned char> rgb;
  int frame_index = 0;
};

class AsyncVideoWriter {
public:
  explicit AsyncVideoWriter(FILE* pipe): pipe_(pipe) {
    worker_ = std::thread([this]() { run(); });
  }

  ~AsyncVideoWriter() {
    finish();
  }

  bool submit(std::vector<unsigned char>&& rgb, int frame_index) {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [&]() {
      return failed_ || queue_.size() < kMaxQueuedFrames;
    });

    if (failed_) {
      return false;
    }

    queue_.push_back(QueuedRgbFrame{std::move(rgb), frame_index});
    cv_.notify_all();
    return true;
  }

  bool finish() {
    std::unique_lock<std::mutex> lock(mutex_);
    if (joined_) {
      return !failed_;
    }

    closed_ = true;
    cv_.notify_all();
    lock.unlock();

    if (worker_.joinable()) {
      worker_.join();
    }

    lock.lock();
    joined_ = true;
    return !failed_;
  }

private:
  static constexpr size_t kMaxQueuedFrames = constant::cuda_renderer::async_video_queue_depth;

  void run() {
    while (true) {
      QueuedRgbFrame frame;

      {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [&]() {
          return failed_ || closed_ || !queue_.empty();
        });

        if (failed_) {
          return;
        }

        if (queue_.empty()) {
          if (closed_) {
            return;
          }
          continue;
        }

        frame = std::move(queue_.front());
        queue_.pop_front();
        cv_.notify_all();
      }

      const size_t expected_bytes = frame.rgb.size();
      const size_t written_bytes = fwrite(frame.rgb.data(), 1, expected_bytes, pipe_);
      if (written_bytes != expected_bytes) {
        std::lock_guard<std::mutex> lock(mutex_);
        failed_ = true;
        std::cerr << "error: failed to write frame "
                  << frame.frame_index << " to ffmpeg\n";
        cv_.notify_all();
        return;
      }
    }
  }

  FILE* pipe_;
  std::thread worker_;
  std::mutex mutex_;
  std::condition_variable cv_;
  std::deque<QueuedRgbFrame> queue_;
  bool closed_ = false;
  bool failed_ = false;
  bool joined_ = false;
};

static std::string format_eta_seconds_cuda(double seconds_total) {
  if (seconds_total < 0.0) seconds_total = 0.0;
  const int total = static_cast<int>(seconds_total + 0.5);
  const int h = total / 3600;
  const int m = (total % 3600) / 60;
  const int s = total % 60;

  std::ostringstream oss;
  if (h > 0) oss << h << "h";
  if (h > 0 || m > 0) oss << m << "m";
  oss << s << "s";
  return oss.str();
}

__host__ __device__ static inline DevVec3 make_v3(float x, float y, float z) {
  DevVec3 v{x, y, z};
  return v;
}

__host__ __device__ static inline DevVec3 add_v3(DevVec3 a, DevVec3 b) {
  return make_v3(a.x + b.x, a.y + b.y, a.z + b.z);
}

__host__ __device__ static inline DevVec3 sub_v3(DevVec3 a, DevVec3 b) {
  return make_v3(a.x - b.x, a.y - b.y, a.z - b.z);
}

__host__ __device__ static inline DevVec3 mul_v3(DevVec3 a, float k) {
  return make_v3(a.x * k, a.y * k, a.z * k);
}

__host__ __device__ static inline float dot_v3(DevVec3 a, DevVec3 b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

__host__ __device__ static inline DevVec3 cross_v3(DevVec3 a, DevVec3 b) {
  return make_v3(
    a.y * b.z - a.z * b.y,
    a.z * b.x - a.x * b.z,
    a.x * b.y - a.y * b.x
  );
}

__host__ __device__ static inline float len2_v3(DevVec3 a) {
  return dot_v3(a, a);
}

__host__ __device__ static inline float len_v3(DevVec3 a) {
  return sqrtf(len2_v3(a));
}

__host__ __device__ static inline DevVec3 norm_v3(DevVec3 a) {
  float l = len_v3(a);
  if (l < constant::cuda_renderer::vec_norm_epsilon) return make_v3(0.0f, 0.0f, 0.0f);
  return mul_v3(a, 1.0f / l);
}

__host__ __device__ static inline float clampf(float x, float lo, float hi) {
  return x < lo ? lo : (x > hi ? hi : x);
}

__host__ __device__ static inline int clampi(int x, int lo, int hi) {
  return x < lo ? lo : (x > hi ? hi : x);
}

__host__ __device__ static inline DevVec3 lerp_v3(DevVec3 a, DevVec3 b, float t) {
  return add_v3(mul_v3(a, 1.0f - t), mul_v3(b, t));
}

__host__ __device__ static inline float brightness_v3(DevVec3 color) {
  return color.x * 0.2126f + color.y * 0.7152f + color.z * 0.0722f;
}

__device__ static inline DeviceFrameCamera make_frame_camera(int frame_index) {
  DeviceFrameCamera cam{};

  const float orbit_radius_xy = constant::camera0::camera_r * cosf(constant::camera0::phi);
  const float orbit_z = constant::camera0::camera_r * sinf(constant::camera0::phi);
  const float orbit_phase =
    (2.0f * constant::math::pi * frame_index) / constant::video::frame_count;

  cam.camera = make_v3(
    orbit_radius_xy * cosf(orbit_phase),
    orbit_radius_xy * sinf(orbit_phase),
    orbit_z
  );

  cam.view_direction = norm_v3(make_v3(-cam.camera.x, -cam.camera.y, -cam.camera.z));

  DevVec3 up_world = make_v3(0.0f, 0.0f, 1.0f);
  if (fabsf(dot_v3(cam.view_direction, up_world)) > 0.999f) {
    up_world = make_v3(0.0f, 1.0f, 0.0f);
  }

  DevVec3 right = norm_v3(cross_v3(up_world, cam.view_direction));
  DevVec3 up = norm_v3(cross_v3(cam.view_direction, right));

  const float sin_alpha = sinf(constant::camera0::alpha);
  const float cos_alpha = cosf(constant::camera0::alpha);

  cam.camera_y = norm_v3(add_v3(mul_v3(right, cos_alpha), mul_v3(up, sin_alpha)));
  cam.camera_x = norm_v3(sub_v3(mul_v3(up, cos_alpha), mul_v3(right, sin_alpha)));
  cam.focal_length = constant::camera0::focal_length;
  cam.cell_length = constant::camera0::width / constant::video::width_px;

  return cam;
}

__device__ static inline DevVec3 get_ray_direction(
  const DeviceFrameCamera& cam,
  int i,
  int j,
  float jitter_i,
  float jitter_j
) {
  DevVec3 image_plane = add_v3(
    add_v3(
      mul_v3(cam.view_direction, cam.focal_length),
      mul_v3(
        cam.camera_x,
        (-constant::video::height_px * 0.5f + i + jitter_i) * cam.cell_length
      )
    ),
    mul_v3(
      cam.camera_y,
      (-constant::video::width_px * 0.5f + j + jitter_j) * cam.cell_length
    )
  );

  return norm_v3(image_plane);
}

__global__ static void bloom_extract_highlight_kernel(
  DevVec3* highlight,
  const DevVec3* frame,
  int pixel_count,
  float threshold
) {
  int idx = blockIdx.x * blockDim.x + threadIdx.x;
  if (idx >= pixel_count) return;

  DevVec3 color = frame[idx];
  highlight[idx] = brightness_v3(color) > threshold
    ? color
    : make_v3(0.0f, 0.0f, 0.0f);
}

__global__ static void bloom_blur_rows_kernel(
  DevVec3* out,
  const DevVec3* in,
  int width,
  int height,
  int kernel_size
) {
  int idx = blockIdx.x * blockDim.x + threadIdx.x;
  int pixel_count = width * height;
  if (idx >= pixel_count) return;

  float radius = kernel_size * 0.5f;
  if (radius <= 0.0f) {
    out[idx] = in[idx];
    return;
  }

  int i = idx / width;
  int j = idx - i * width;
  int radius_i = (int)radius;

  DevVec3 sum = make_v3(0.0f, 0.0f, 0.0f);
  float weight_sum = 0.0f;
  for (int k = -radius_i; k <= radius_i; ++k) {
    int ii = i + k;
    if (ii < 0 || ii >= height) continue;

    float weight = expf(-(float)(k * k) / (2.0f * radius * radius));
    sum = add_v3(sum, mul_v3(in[ii * width + j], weight));
    weight_sum += weight;
  }

  out[idx] = mul_v3(sum, 1.0f / weight_sum);
}

__global__ static void bloom_blur_cols_kernel(
  DevVec3* out,
  const DevVec3* in,
  int width,
  int height,
  int kernel_size
) {
  int idx = blockIdx.x * blockDim.x + threadIdx.x;
  int pixel_count = width * height;
  if (idx >= pixel_count) return;

  float radius = kernel_size * 0.5f;
  if (radius <= 0.0f) {
    out[idx] = in[idx];
    return;
  }

  int i = idx / width;
  int j = idx - i * width;
  int radius_i = (int)radius;

  DevVec3 sum = make_v3(0.0f, 0.0f, 0.0f);
  float weight_sum = 0.0f;
  for (int k = -radius_i; k <= radius_i; ++k) {
    int jj = j + k;
    if (jj < 0 || jj >= width) continue;

    float weight = expf(-(float)(k * k) / (2.0f * radius * radius));
    sum = add_v3(sum, mul_v3(in[i * width + jj], weight));
    weight_sum += weight;
  }

  out[idx] = mul_v3(sum, 1.0f / weight_sum);
}

__global__ static void bloom_accumulate_kernel(
  DevVec3* frame,
  const DevVec3* blurred,
  int pixel_count,
  float coeff
) {
  int idx = blockIdx.x * blockDim.x + threadIdx.x;
  if (idx >= pixel_count) return;

  frame[idx] = add_v3(frame[idx], mul_v3(blurred[idx], coeff));
}

__device__ static inline unsigned char float_to_u8(float value) {
  value = clampf(value, 0.0f, 1.0f);
  return static_cast<unsigned char>(value * 255.99f);
}

__global__ static void pack_rgb24_kernel(
  unsigned char* rgb,
  const DevVec3* frame,
  int pixel_count
) {
  int idx = blockIdx.x * blockDim.x + threadIdx.x;
  if (idx >= pixel_count) return;

  DevVec3 color = frame[idx];
  int base = idx * constant::output::raw_rgb_channels;
  rgb[base + 0] = float_to_u8(color.x);
  rgb[base + 1] = float_to_u8(color.y);
  rgb[base + 2] = float_to_u8(color.z);
}

__device__ static inline uint32_t xorshift32(uint32_t& s) {
  s ^= s << 13;
  s ^= s >> 17;
  s ^= s << 5;
  return s;
}

__device__ static inline float rng_f01(uint32_t& s) {
  return (xorshift32(s) & 0x00ffffff) * (1.0f / 16777216.0f);
}

__device__ static inline float sample_shape_noise(
  const float* tex, const DeviceScene& sc, float x, float y
) {
  float u = (x + sc.Rout) / (2.0f * sc.Rout);
  float v = (y + sc.Rout) / (2.0f * sc.Rout);
  u = clampf(u, 0.0f, 1.0f);
  v = clampf(v, 0.0f, 1.0f);

  float fx = u * (sc.shape_w - 1);
  float fy = v * (sc.shape_h - 1);

  int x0 = (int)fx;
  int y0 = (int)fy;
  int x1 = min(x0 + 1, sc.shape_w - 1);
  int y1 = min(y0 + 1, sc.shape_h - 1);

  float tx = fx - x0;
  float ty = fy - y0;

  float c00 = tex[y0 * sc.shape_w + x0];
  float c10 = tex[y0 * sc.shape_w + x1];
  float c01 = tex[y1 * sc.shape_w + x0];
  float c11 = tex[y1 * sc.shape_w + x1];

  float a = c00 * (1.0f - tx) + c10 * tx;
  float b = c01 * (1.0f - tx) + c11 * tx;
  return a * (1.0f - ty) + b * ty;
}

__device__ static inline float sample_cloud_noise(
  const float* tex, const DeviceScene& sc, float x, float y, float z
) {
  float u = (x + sc.Rout) / (2.0f * sc.Rout);
  float v = (y + sc.Rout) / (2.0f * sc.Rout);
  float w = (z + 0.5f * sc.disk_thickness) / sc.disk_thickness;

  u = clampf(u, 0.0f, 1.0f);
  v = clampf(v, 0.0f, 1.0f);
  w = clampf(w, 0.0f, 1.0f);

  float fx = u * (sc.cloud_x - 1);
  float fy = v * (sc.cloud_y - 1);
  float fz = w * (sc.cloud_z - 1);

  int x0 = (int)fx, y0 = (int)fy, z0 = (int)fz;
  int x1 = min(x0 + 1, sc.cloud_x - 1);
  int y1 = min(y0 + 1, sc.cloud_y - 1);
  int z1 = min(z0 + 1, sc.cloud_z - 1);

  float tx = fx - x0, ty = fy - y0, tz = fz - z0;

  auto at = [&](int zz, int yy, int xx) -> float {
    return tex[(zz * sc.cloud_y + yy) * sc.cloud_x + xx];
  };

  float c000 = at(z0, y0, x0);
  float c100 = at(z0, y0, x1);
  float c010 = at(z0, y1, x0);
  float c110 = at(z0, y1, x1);
  float c001 = at(z1, y0, x0);
  float c101 = at(z1, y0, x1);
  float c011 = at(z1, y1, x0);
  float c111 = at(z1, y1, x1);

  float c00 = c000 * (1.0f - tx) + c100 * tx;
  float c10 = c010 * (1.0f - tx) + c110 * tx;
  float c01 = c001 * (1.0f - tx) + c101 * tx;
  float c11 = c011 * (1.0f - tx) + c111 * tx;

  float c0 = c00 * (1.0f - ty) + c10 * ty;
  float c1 = c01 * (1.0f - ty) + c11 * ty;

  return c0 * (1.0f - tz) + c1 * tz;
}

__device__ static inline DevVec3 sample_disk_color(
  const DevVec3* lut, const DeviceScene& sc, float r
) {
  float t = (r - sc.Rin) / (sc.Rout - sc.Rin);
  t = clampf(t, 0.0f, 1.0f);

  float fi = t * (sc.color_n - 1);
  int i0 = (int)fi;
  int i1 = min(i0 + 1, sc.color_n - 1);
  float w = fi - i0;

  return lerp_v3(lut[i0], lut[i1], w);
}

__device__ static inline DevVec3 background_get_pixel(
  const unsigned char* tex, int w, int h, int x, int y
) {
  x = clampi(x, 0, h - 1);
  y %= w;
  if (y < 0) y += w;

  const unsigned char* p = tex + (x * w + y) * 3;
  return make_v3(
    p[0] * (1.0f / 255.0f),
    p[1] * (1.0f / 255.0f),
    p[2] * (1.0f / 255.0f)
  );
}

__device__ static inline DevVec3 sample_background(
  const unsigned char* tex, const DeviceScene& sc, DevVec3 direction
) {
  float l = len_v3(direction);
  if (l < constant::cuda_renderer::background_dir_epsilon) return make_v3(0.0f, 0.0f, 0.0f);

  float theta = atan2f(direction.y, direction.x);
  float phi = acosf(clampf(direction.z / l, -1.0f, 1.0f));

  float u = theta / (2.0f * constant::math::pi) + 0.5f;
  float v = phi / constant::math::pi;

  float fx = v * (sc.bg_h - 1);
  float fy = u * sc.bg_w;

  int x0 = min((int)floorf(fx), sc.bg_h - 1);
  int y0_unwrapped = (int)floorf(fy);
  int y0 = y0_unwrapped % sc.bg_w;
  int x1 = min(x0 + 1, sc.bg_h - 1);
  if (y0 < 0) y0 += sc.bg_w;
  int y1 = (y0 + 1) % sc.bg_w;

  float wx = fx - x0;
  float wy = fy - floorf(fy);

  DevVec3 c00 = background_get_pixel(tex, sc.bg_w, sc.bg_h, x0, y0);
  DevVec3 c01 = background_get_pixel(tex, sc.bg_w, sc.bg_h, x0, y1);
  DevVec3 c10 = background_get_pixel(tex, sc.bg_w, sc.bg_h, x1, y0);
  DevVec3 c11 = background_get_pixel(tex, sc.bg_w, sc.bg_h, x1, y1);

  DevVec3 c0 = lerp_v3(c00, c01, wy);
  DevVec3 c1 = lerp_v3(c10, c11, wy);
  return lerp_v3(c0, c1, wx);
}

__device__ static inline float get_dl(const DeviceScene& sc, DevVec3 pos) {
  return sc.dl0 * fminf(
    len_v3(pos),
    fabsf(pos.z) / sc.disk_thickness / constant::cuda_renderer::dl_z_scale
      + constant::cuda_renderer::dl_z_bias
  );
}

__device__ static inline void step_sm_ray(DevVec3& pos, DevVec3& dir, float dl) {
  DevVec3 r = make_v3(-pos.x, -pos.y, -pos.z);
  float r2 = len2_v3(r);
  if (r2 < constant::cuda_renderer::trace_hit_epsilon) {
    pos = add_v3(pos, mul_v3(dir, dl));
    return;
  }

  DevVec3 normal = norm_v3(r);
  float h2 = len2_v3(cross_v3(r, dir));
  DevVec3 dv = mul_v3(normal, (1.5f * h2 / (r2 * r2)) * dl);

  dir = norm_v3(add_v3(dir, dv));
  pos = add_v3(pos, mul_v3(dir, dl));
}

__device__ static DevVec3 trace_one(
  DevVec3 start,
  DevVec3 direction,
  uint32_t seed,
  const DeviceScene& sc,
  const unsigned char* bg,
  const float* shape_noise,
  const float* cloud_noise,
  const DevVec3* color_lut
) {
  DevVec3 color = make_v3(0.0f, 0.0f, 0.0f);
  float alpha = 1.0f;

  DevVec3 pos = start;
  DevVec3 dir = norm_v3(direction);

  step_sm_ray(pos, dir, get_dl(sc, pos) * rng_f01(seed));

  for (int step_n = 0; step_n < sc.step_limit; ++step_n) {
    float pos_l2 = len2_v3(pos);

    if (pos_l2 < constant::cuda_renderer::capture_radius * constant::cuda_renderer::capture_radius) {
      break;
    }

    if (pos_l2 >= sc.outer_range2) {
      DevVec3 bgc = sample_background(bg, sc, dir);
      color = add_v3(color, mul_v3(bgc, alpha));
      break;
    }

    float r = sqrtf(pos.x * pos.x + pos.y * pos.y);
    float dl = get_dl(sc, pos);

    if (fabsf(pos.z) < sc.disk_thickness / 2.0f && r < sc.Rout && r > sc.Rin) {
      float shape = sample_shape_noise(shape_noise, sc, pos.x, pos.y);
      float thickness = sc.disk_thickness * (
        constant::cuda_renderer::disk_thickness_base
        + constant::cuda_renderer::disk_thickness_shape_scale * shape
      );

      float thickness_factor = fminf(1.0f, 1.0f - fabsf(pos.z / thickness));
      float l = fminf(sc.Rout - r, r - sc.Rin);
      float edge_factor = 1.0f - 1.0f / (constant::cuda_renderer::disk_edge_falloff * l + 1.0f);

      float density = constant::cuda_renderer::disk_density_scale * edge_factor * thickness_factor
        * sample_cloud_noise(cloud_noise, sc, pos.x, pos.y, pos.z);
      if (density < 0.0f) density = 0.0f;

      DevVec3 S = mul_v3(
        sample_disk_color(color_lut, sc, r),
        sc.disk_luminous_intensity
      );

      float tau = density * sc.disk_opacity * dl;
      float T = expf(-tau);

      color = add_v3(color, mul_v3(S, alpha * (1.0f - T)));
      alpha *= T;

      if (alpha < constant::cuda_renderer::alpha_stop_threshold) break;
    }

    step_sm_ray(pos, dir, dl);
  }

  return color;
}

__global__ static void render_kernel(
  DevVec3* batch_frames,
  int job_count,
  int frame_begin,
  int cur_frames,
  int tile_i0,
  int tile_j0,
  int tile_h,
  int tile_w,
  int frame_stride,
  DeviceScene sc,
  const unsigned char* bg,
  const float* shape_noise,
  const float* cloud_noise,
  const DevVec3* color_lut
) {
  int idx = blockIdx.x * blockDim.x + threadIdx.x;
  if (idx >= job_count) return;

  const int pixel_slot = idx / cur_frames;
  const int frame_offset = idx - pixel_slot * cur_frames;
  const int di = pixel_slot / tile_w;
  const int dj = pixel_slot - di * tile_w;

  const int pixel_i = tile_i0 + di;
  const int pixel_j = tile_j0 + dj;
  const int frame_index = frame_begin + frame_offset;

  const DeviceFrameCamera cam = make_frame_camera(frame_index);
  DevVec3 start = cam.camera;

  uint32_t pixel_seed =
    sc.seed
    ^ ((uint32_t)pixel_i * 1315423911u)
    ^ ((uint32_t)pixel_j * 2654435761u)
    ^ ((uint32_t)frame_index * 374761393u);

  DevVec3 acc = make_v3(0.0f, 0.0f, 0.0f);
  for (unsigned s = 0; s < sc.taa_times; ++s) {
    uint32_t state = pixel_seed ^ (s * 2246822519u);
    DevVec3 dir = get_ray_direction(
      cam,
      pixel_i,
      pixel_j,
      rng_f01(state),
      rng_f01(state)
    );
    acc = add_v3(acc, trace_one(start, dir, state, sc, bg, shape_noise, cloud_noise, color_lut));
  }

  const int global_index = frame_offset * frame_stride + pixel_i * constant::video::width_px + pixel_j;
  batch_frames[global_index] = mul_v3(acc, 1.0f / (float)sc.taa_times);
}

static void ensure_batch_frame_capacity(int n) {
  if (n <= g.batch_frame_capacity) return;

  if (g.d_batch_frames) CUDA_CHECK(cudaFree(g.d_batch_frames));

  CUDA_CHECK(cudaMalloc(&g.d_batch_frames, sizeof(DevVec3) * n));
  g.batch_frame_capacity = n;
}

static void ensure_postprocess_slot_capacity(PostprocessSlot& slot, int pixel_count) {
  const int byte_count = pixel_count * constant::output::raw_rgb_channels;

  if (!slot.stream) {
    CUDA_CHECK(cudaStreamCreate(&slot.stream));
  }

  if (pixel_count > slot.frame_capacity) {
    if (slot.d_frame) CUDA_CHECK(cudaFree(slot.d_frame));
    if (slot.d_highlight) CUDA_CHECK(cudaFree(slot.d_highlight));
    if (slot.d_blur_tmp) CUDA_CHECK(cudaFree(slot.d_blur_tmp));
    if (slot.d_blur_out) CUDA_CHECK(cudaFree(slot.d_blur_out));

    CUDA_CHECK(cudaMalloc(&slot.d_frame, sizeof(DevVec3) * pixel_count));
    CUDA_CHECK(cudaMalloc(&slot.d_highlight, sizeof(DevVec3) * pixel_count));
    CUDA_CHECK(cudaMalloc(&slot.d_blur_tmp, sizeof(DevVec3) * pixel_count));
    CUDA_CHECK(cudaMalloc(&slot.d_blur_out, sizeof(DevVec3) * pixel_count));
    slot.frame_capacity = pixel_count;
  }

  if (byte_count > slot.rgb24_capacity) {
    if (slot.d_rgb24) CUDA_CHECK(cudaFree(slot.d_rgb24));
    if (slot.h_rgb24) CUDA_CHECK(cudaFreeHost(slot.h_rgb24));

    CUDA_CHECK(cudaMalloc(&slot.d_rgb24, byte_count));
    CUDA_CHECK(cudaHostAlloc(&slot.h_rgb24, byte_count, cudaHostAllocDefault));
    slot.rgb24_capacity = byte_count;
  }
}

template <size_t N>
static void launch_postprocess_cuda(
  PostprocessSlot& slot,
  const DevVec3* source_frame,
  const std::array<BloomConfig, N>& configs
) {
  const int width = constant::video::width_px;
  const int height = constant::video::height_px;
  const int pixel_count = width * height;
  const int byte_count = pixel_count * constant::output::raw_rgb_channels;
  const int blocks = (pixel_count + constant::cuda_renderer::threads - 1)
    / constant::cuda_renderer::threads;

  ensure_postprocess_slot_capacity(slot, pixel_count);

  CUDA_CHECK(cudaMemcpyAsync(
    slot.d_frame,
    source_frame,
    sizeof(DevVec3) * pixel_count,
    cudaMemcpyDeviceToDevice,
    slot.stream
  ));

  bloom_extract_highlight_kernel<<<blocks, constant::cuda_renderer::threads, 0, slot.stream>>>(
    slot.d_highlight,
    slot.d_frame,
    pixel_count,
    constant::cuda_renderer::bloom_highlight_threshold
  );
  CUDA_CHECK(cudaGetLastError());

  for (const auto& config : configs) {
    bloom_blur_rows_kernel<<<blocks, constant::cuda_renderer::threads, 0, slot.stream>>>(
      slot.d_blur_tmp,
      slot.d_highlight,
      width,
      height,
      config.gaussian_kernel_size
    );
    CUDA_CHECK(cudaGetLastError());

    bloom_blur_cols_kernel<<<blocks, constant::cuda_renderer::threads, 0, slot.stream>>>(
      slot.d_blur_out,
      slot.d_blur_tmp,
      width,
      height,
      config.gaussian_kernel_size
    );
    CUDA_CHECK(cudaGetLastError());

    bloom_accumulate_kernel<<<blocks, constant::cuda_renderer::threads, 0, slot.stream>>>(
      slot.d_frame,
      slot.d_blur_out,
      pixel_count,
      config.superposition_coeff
    );
    CUDA_CHECK(cudaGetLastError());
  }

  pack_rgb24_kernel<<<blocks, constant::cuda_renderer::threads, 0, slot.stream>>>(
    slot.d_rgb24,
    slot.d_frame,
    pixel_count
  );
  CUDA_CHECK(cudaGetLastError());
  CUDA_CHECK(cudaMemcpyAsync(
    slot.h_rgb24,
    slot.d_rgb24,
    byte_count,
    cudaMemcpyDeviceToHost,
    slot.stream
  ));
}

static bool finalize_postprocess_slot(PostprocessSlot& slot, AsyncVideoWriter& video_writer) {
  if (slot.pending_frame_index < 0) return true;

  CUDA_CHECK(cudaStreamSynchronize(slot.stream));

  std::vector<unsigned char> rgb24(slot.h_rgb24, slot.h_rgb24 + slot.rgb24_capacity);
  const int frame_index = slot.pending_frame_index;
  slot.pending_frame_index = -1;
  return video_writer.submit(std::move(rgb24), frame_index);
}

static void init_cuda_assets() {
  if (g.ready) return;

  int bg_w = 0, bg_h = 0, bg_c = 0;
  unsigned char* background = stbi_load("assets/eso0932a.jpg", &bg_w, &bg_h, &bg_c, 3);
  if (!background) {
    std::cerr << "error: failed to load assets/eso0932a.jpg\n";
    std::abort();
  }

  g.bg_w = bg_w;
  g.bg_h = bg_h;

  CUDA_CHECK(cudaMalloc(&g.d_background, bg_w * bg_h * constant::output::raw_rgb_channels));
  CUDA_CHECK(cudaMemcpy(
    g.d_background,
    background,
    bg_w * bg_h * constant::output::raw_rgb_channels,
    cudaMemcpyHostToDevice
  ));
  stbi_image_free(background);

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

  std::vector<float> shape_lut(
    constant::cuda_renderer::shape_noise_w * constant::cuda_renderer::shape_noise_h
  );
  for (int y = 0; y < constant::cuda_renderer::shape_noise_h; ++y) {
    float py = -constant::model::Rout
      + (2.0f * constant::model::Rout) * (float)y
        / (float)(constant::cuda_renderer::shape_noise_h - 1);
    for (int x = 0; x < constant::cuda_renderer::shape_noise_w; ++x) {
      float px = -constant::model::Rout
        + (2.0f * constant::model::Rout) * (float)x
          / (float)(constant::cuda_renderer::shape_noise_w - 1);
      shape_lut[y * constant::cuda_renderer::shape_noise_w + x] =
        shape_noise.get_noise(Vec3(px, py, 0.0f));
    }
  }

  std::vector<float> cloud_lut(
    constant::cuda_renderer::cloud_noise_x
    * constant::cuda_renderer::cloud_noise_y
    * constant::cuda_renderer::cloud_noise_z
  );
  for (int z = 0; z < constant::cuda_renderer::cloud_noise_z; ++z) {
    float pz = -constant::model::disk_thickness * 0.5f
      + constant::model::disk_thickness * (float)z
        / (float)(constant::cuda_renderer::cloud_noise_z - 1);
    for (int y = 0; y < constant::cuda_renderer::cloud_noise_y; ++y) {
      float py = -constant::model::Rout
        + (2.0f * constant::model::Rout) * (float)y
          / (float)(constant::cuda_renderer::cloud_noise_y - 1);
      for (int x = 0; x < constant::cuda_renderer::cloud_noise_x; ++x) {
        float px = -constant::model::Rout
          + (2.0f * constant::model::Rout) * (float)x
            / (float)(constant::cuda_renderer::cloud_noise_x - 1);
        cloud_lut[(z * constant::cuda_renderer::cloud_noise_y + y)
          * constant::cuda_renderer::cloud_noise_x + x] =
          cloud_noise.get_noise(Vec3(px, py, pz));
      }
    }
  }

  std::vector<DevVec3> color_lut(constant::cuda_renderer::color_lut_n);
  const float max_temp = color::maximum_temperature();
  for (int i = 0; i < constant::cuda_renderer::color_lut_n; ++i) {
    float t = (float)i / (float)(constant::cuda_renderer::color_lut_n - 1);
    float r = constant::model::Rin + t * (constant::model::Rout - constant::model::Rin);

    float temperature = color::get_disk_temperature(r * constant::model::Rs_m);
    temperature = max_temp - constant::cuda_renderer::lut_temperature_shift * (max_temp - temperature);

    Vec3 c = color::black_body_color(temperature);
    float brightness = constant::cuda_renderer::lut_brightness_scale * temperature / max_temp;

    color_lut[i] = DevVec3{
      c.x * brightness,
      c.y * brightness,
      c.z * brightness
    };
  }

  CUDA_CHECK(cudaMalloc(&g.d_shape_noise, sizeof(float) * shape_lut.size()));
  CUDA_CHECK(cudaMalloc(&g.d_cloud_noise, sizeof(float) * cloud_lut.size()));
  CUDA_CHECK(cudaMalloc(&g.d_color_lut, sizeof(DevVec3) * color_lut.size()));

  CUDA_CHECK(cudaMemcpy(
    g.d_shape_noise, shape_lut.data(),
    sizeof(float) * shape_lut.size(),
    cudaMemcpyHostToDevice
  ));
  CUDA_CHECK(cudaMemcpy(
    g.d_cloud_noise, cloud_lut.data(),
    sizeof(float) * cloud_lut.size(),
    cudaMemcpyHostToDevice
  ));
  CUDA_CHECK(cudaMemcpy(
    g.d_color_lut, color_lut.data(),
    sizeof(DevVec3) * color_lut.size(),
    cudaMemcpyHostToDevice
  ));

  g.scene.outer_range2 = constant::renderer::outer_range * constant::renderer::outer_range;
  g.scene.step_limit = constant::renderer::step_limit;
  g.scene.dl0 = constant::renderer::dl0;

  g.scene.disk_thickness = constant::model::disk_thickness;
  g.scene.Rin = constant::model::Rin;
  g.scene.Rout = constant::model::Rout;
  g.scene.Rs_m = constant::model::Rs_m;
  g.scene.disk_luminous_intensity = constant::model::disk_luminous_intensity;
  g.scene.disk_opacity = constant::model::disk_opacity;

  g.scene.seed = constant::cuda_renderer::seed;
  g.scene.taa_times = constant::cuda_renderer::taa_times;

  g.scene.bg_w = bg_w;
  g.scene.bg_h = bg_h;

  g.scene.shape_w = constant::cuda_renderer::shape_noise_w;
  g.scene.shape_h = constant::cuda_renderer::shape_noise_h;

  g.scene.cloud_x = constant::cuda_renderer::cloud_noise_x;
  g.scene.cloud_y = constant::cuda_renderer::cloud_noise_y;
  g.scene.cloud_z = constant::cuda_renderer::cloud_noise_z;

  g.scene.color_n = constant::cuda_renderer::color_lut_n;

  g.ready = true;
}

} // namespace

int render_video_cuda(FILE* video_pipe) {
  using namespace constant;

  init_cuda_assets();
  AsyncVideoWriter video_writer(video_pipe);

  const std::array<BloomConfig, constant::cuda_renderer::bloom_radius_scales.size()> bloom_cfg{{
    BloomConfig{
      constant::cuda_renderer::bloom_superposition_coeffs[0],
      static_cast<int>(1 + constant::cuda_renderer::bloom_radius_scales[0]
        * (video::width_px / constant::cuda_renderer::bloom_reference_widths[0]))
    },
    BloomConfig{
      constant::cuda_renderer::bloom_superposition_coeffs[1],
      static_cast<int>(1 + constant::cuda_renderer::bloom_radius_scales[1]
        * (video::width_px / constant::cuda_renderer::bloom_reference_widths[1]))
    },
    BloomConfig{
      constant::cuda_renderer::bloom_superposition_coeffs[2],
      static_cast<int>(1 + constant::cuda_renderer::bloom_radius_scales[2]
        * (video::width_px / constant::cuda_renderer::bloom_reference_widths[2]))
    }
  }};

  const auto render_start = std::chrono::steady_clock::now();
  auto submit_completed_frame = [&](PostprocessSlot& slot) -> bool {
    if (slot.pending_frame_index < 0) return true;
    const int frames_done = slot.pending_frame_index + 1;
    if (!finalize_postprocess_slot(slot, video_writer)) {
      return false;
    }

    const auto now = std::chrono::steady_clock::now();
    const double elapsed_seconds =
      std::chrono::duration<double>(now - render_start).count();
    const double seconds_per_frame = elapsed_seconds / frames_done;
    const double eta_seconds = seconds_per_frame * (video::frame_count - frames_done);

    std::cout
      << "frame " << frames_done << '/' << video::frame_count
      << " done, eta " << format_eta_seconds_cuda(eta_seconds)
      << std::endl;
    return true;
  };

  for (
    int frame_begin = 0;
    frame_begin < video::frame_count;
    frame_begin += constant::cuda_renderer::frame_batch
  ) {
    const int cur_frames = std::min(
      constant::cuda_renderer::frame_batch,
      video::frame_count - frame_begin
    );

    const int frame_stride = video::height_px * video::width_px;
    ensure_batch_frame_capacity(cur_frames * frame_stride);

    for (int i0 = 0; i0 < video::height_px; i0 += constant::cuda_renderer::tile_h) {
      for (int j0 = 0; j0 < video::width_px; j0 += constant::cuda_renderer::tile_w) {
        const int th = std::min(constant::cuda_renderer::tile_h, video::height_px - i0);
        const int tw = std::min(constant::cuda_renderer::tile_w, video::width_px - j0);
        const int tile_pixels = th * tw;
        const int job_count = tile_pixels * cur_frames;

        const int blocks = (job_count + constant::cuda_renderer::threads - 1)
          / constant::cuda_renderer::threads;
        render_kernel<<<blocks, constant::cuda_renderer::threads>>>(
          g.d_batch_frames,
          job_count,
          frame_begin,
          cur_frames,
          i0,
          j0,
          th,
          tw,
          frame_stride,
          g.scene,
          g.d_background,
          g.d_shape_noise,
          g.d_cloud_noise,
          g.d_color_lut
        );
        CUDA_CHECK(cudaGetLastError());
      }
    }

    CUDA_CHECK(cudaDeviceSynchronize());

    for (int f = 0; f < cur_frames; ++f) {
      PostprocessSlot& slot = g_post_slots[f % g_post_slots.size()];
      if (!submit_completed_frame(slot)) {
        return 0;
      }

      launch_postprocess_cuda(
        slot,
        g.d_batch_frames + (size_t)f * frame_stride,
        bloom_cfg
      );
      slot.pending_frame_index = frame_begin + f;
    }

    while (true) {
      PostprocessSlot* next_slot = nullptr;
      for (auto& slot : g_post_slots) {
        if (slot.pending_frame_index < 0) continue;
        if (!next_slot || slot.pending_frame_index < next_slot->pending_frame_index) {
          next_slot = &slot;
        }
      }

      if (!next_slot) {
        break;
      }

      if (!submit_completed_frame(*next_slot)) {
        return 0;
      }
    }
  }

  if (!video_writer.finish()) {
    return 0;
  }

  return 1;
}