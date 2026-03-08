#ifndef BLOOM_HPP__
#define BLOOM_HPP__

#include "../utils/image.hpp"
#include "../utils/bar.hpp"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <mutex>
#include <thread>
#include <vector>

struct BloomConfig {
  BloomConfig(
    float superposition_coeff,
    int gaussian_kernel_size
  ):
    superposition_coeff(superposition_coeff),
    gaussian_kernel_size(gaussian_kernel_size) {}

  float superposition_coeff;
  int gaussian_kernel_size;
};

template <size_t Height, size_t Width, size_t Times>
void renderer_bloom(
  Image<Height, Width>& buffer,
  const Image<Height, Width>& origin,
  std::array<BloomConfig, Times> configures
);


static float brightness(Vec3 color) {
  static const Vec3 k(0.2126, 0.7152, 0.0722);
  return color % k;
}

template <size_t Height, size_t Width>
static void gaussian_blur(
  Image<Height, Width>& buffer, 
  const Image<Height, Width>& origin, 
  int kernel_size
) {
  float radius = kernel_size / 2.;

  if (radius <= 0.0f) {
    buffer = origin;
    return;
  }

  static Image<Height, Width> tmp;

  auto thread_count = []() -> unsigned {
    unsigned n = std::thread::hardware_concurrency();
    return n ? n : 1;
  };

  constexpr int rows_per_job = 16;
  constexpr int cols_per_job = 16;

  const int row_jobs = ((int)Height + rows_per_job - 1) / rows_per_job;
  const int col_jobs = ((int)Width + cols_per_job - 1) / cols_per_job;
  const int total_jobs = row_jobs + col_jobs;

  Bar bar(total_jobs);
  std::mutex bar_mutex;

  const unsigned n_threads = thread_count();

  auto step_bar = [&]() {
    std::lock_guard<std::mutex> lock(bar_mutex);
    bar.step();
  };

  auto run_phase = [&](int job_count, auto&& fn) {
    std::atomic<int> next_job{0};

    auto worker = [&]() {
      while (true) {
        const int job = next_job.fetch_add(1, std::memory_order_relaxed);
        if (job >= job_count) break;

        // spec: step on job claim
        step_bar();
        fn(job);
      }
    };

    std::vector<std::thread> threads;
    threads.reserve(n_threads);
    for (unsigned t = 0; t < n_threads; t++) threads.emplace_back(worker);
    for (auto& th : threads) th.join();
  };

  run_phase(row_jobs, [&](int job) {
    const int i0 = job * rows_per_job;
    const int i1 = std::min(i0 + rows_per_job, (int)Height);

    for (int i = i0; i < i1; i++) {
      for (int j = 0; j < (int)Width; j++) {
        Vec3 sum = Vec3::zero();
        float weight_sum = 0.0f;

        for (int k = (int)-radius; k <= (int)radius; ++k) {
          if (i + k < 0 || i + k >= (int)Height) continue;

          float weight = std::exp(-k * k / (2.0f * radius * radius));
          sum += origin[i + k][j] * weight;
          weight_sum += weight;
        }

        tmp[i][j] = sum / weight_sum;
      }
    }
  });

  run_phase(col_jobs, [&](int job) {
    const int j0 = job * cols_per_job;
    const int j1 = std::min(j0 + cols_per_job, (int)Width);

    for (int j = j0; j < j1; j++) {
      for (int i = 0; i < (int)Height; i++) {
        Vec3 sum = Vec3::zero();
        float weight_sum = 0.0f;

        for (int k = (int)-radius; k <= (int)radius; ++k) {
          if (j + k < 0 || j + k >= (int)Width) continue;

          float weight = std::exp(-k * k / (2.0f * radius * radius));
          sum += tmp[i][j + k] * weight;
          weight_sum += weight;
        }

        buffer[i][j] = sum / weight_sum;
      }
    }
  });
}

template <size_t Height, size_t Width, size_t Times>
void renderer_bloom(
  Image<Height, Width>& buffer,
  const Image<Height, Width>& origin,
  std::array<BloomConfig, Times> configures
) {
  buffer = origin;

  static Image<Height, Width> highlight;
  for (int i = 0; i < Height; i++) {
    for (int j = 0; j < Width; j++) {
      highlight[i][j] = brightness(origin[i][j]) > 1.2
        ? origin[i][j]
        : Vec3::zero();
    }
  }

  for (const auto& config: configures) {
    static Image<Height, Width> after_blurred;
    gaussian_blur(after_blurred, highlight, config.gaussian_kernel_size);
    for (int i = 0; i < Height; i++) {
      for (int j = 0; j < Width; j++) {
        buffer[i][j] += after_blurred[i][j] * config.superposition_coeff;
      }
    }
  }
}

#endif