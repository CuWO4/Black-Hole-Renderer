#ifndef LIGHT_CAST_HPP__
#define LIGHT_CAST_HPP__

#include "ray.h"
#include "camera.h"
#include "schwarz_metric_ray.h"

#include "../utils/image.hpp"
#include "../utils/bar.hpp"

#include <functional>

template <size_t Height, size_t Width>
void light_cast_render(
  Image<Height, Width>& image,
  Camera camera,
  std::function<Vec3(Ray&)> get_color_of_ray_f,
  unsigned TAA_times = 5
);


template <size_t Height, size_t Width>
void light_cast_render(
  Image<Height, Width>& image,
  Camera camera,
  std::function<Vec3(Ray&)> get_color_of_ray_f,
  unsigned TAA_times
) {
  Bar bar(Height * Width);
  for (int i = 0; i < Height; i++) {
    for (int j = 0; j < Width; j++) {
      Vec3 color = Vec3::zero();
      for (unsigned _ = 0; _ < TAA_times; _++) {
        SM_Ray ray(
          camera.get_camera(), 
          camera.get_direction_of_i_j(i, j), 
          Vec3::origin()
        );
        color += get_color_of_ray_f(ray);
      }
      image[i][j] = color / TAA_times;
      bar.step();
    }
  }
}

#endif