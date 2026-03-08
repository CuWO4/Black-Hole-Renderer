#ifndef LIGHT_CAST_TILED_MT_HPP__
#define LIGHT_CAST_TILED_MT_HPP__

#include "../camera.h"
#include "../schwarz_metric_ray.h"
#include "../../utils/image.hpp"
#include "../../utils/bar.hpp"
#include "../utils/rng.hpp"

#include <atomic>
#include <cstdint>
#include <mutex>
#include <thread>
#include <vector>
#include <algorithm>

struct RenderSettings {
  unsigned taa_times = 5;
  unsigned thread_n = 0;      // 0 => hardware_concurrency
  int tile_h = 50;
  int tile_w = 50;
  uint32_t seed = 1;
};

struct Tile {
  int i0, i1;
  int j0, j1;
};

template <size_t Height, size_t Width, class Scene>
void light_cast_render_tiled_mt(
  Image<Height, Width>& image,
  Camera& camera,
  Scene&& scene,
  RenderSettings& settings
) {
  int tile_h = std::max(1, settings.tile_h);
  int tile_w = std::max(1, settings.tile_w);

  std::vector<Tile> tiles;
  tiles.reserve(((int)Height + tile_h - 1) / tile_h * (((int)Width + tile_w - 1) / tile_w));
  for (int i = 0; i < (int)Height; i += tile_h) {
    for (int j = 0; j < (int)Width; j += tile_w) {
      tiles.push_back(Tile{
        i, std::min(i + tile_h, (int)Height),
        j, std::min(j + tile_w, (int)Width)
      });
    }
  }

  std::atomic<size_t> next_tile{0};

  Bar bar((int)tiles.size());
  std::mutex bar_mutex;

  unsigned thread_n = settings.thread_n ? settings.thread_n : std::thread::hardware_concurrency();
  if (thread_n == 0) thread_n = 1;

  auto worker = [&](unsigned tid) {
    uint32_t local_seed = settings.seed ^ (0x9e3779b9u * (tid + 1));

    while (true) {
      size_t t = next_tile.fetch_add(1, std::memory_order_relaxed);
      if (t >= tiles.size()) break;

      const Tile tile = tiles[t];

      for (int i = tile.i0; i < tile.i1; i++) {
        for (int j = tile.j0; j < tile.j1; j++) {
          Vec3 c = Vec3::zero();

          uint32_t pixel_seed = local_seed ^ (uint32_t)(i * 1315423911u) ^ (uint32_t)(j * 2654435761u);

          for (unsigned s = 0; s < settings.taa_times; s++) {
            XorShift32Rng rng(pixel_seed ^ (s * 2246822519u));

            SM_Ray ray(
              camera.get_camera(),
              camera.get_direction_of_i_j(i, j),
              Vec3::origin()
            );

            c += scene(ray, rng);
          }

          image[i][j] = c / (float)settings.taa_times;
        }
      }

      {
        std::lock_guard<std::mutex> lock(bar_mutex);
        bar.step();
      }
    }
  };

  std::vector<std::thread> threads;
  threads.reserve(thread_n);
  for (unsigned t = 0; t < thread_n; t++) {
    threads.emplace_back(worker, t);
  }
  for (auto& th : threads) th.join();
}

#endif