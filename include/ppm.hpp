#ifndef PPM_H_
#define PPM_H_

#include "vec3.hpp"

#include <string>
#include <array>

template <size_t width, size_t height>
int output_ppm(std::string file_path, std::array<std::array<Vec3, width>, height>& pixels);



template <size_t width, size_t height>
int output_ppm(std::string file_path, std::array<std::array<Vec3, width>, height>& pixels) {
  constexpr int max_rgb_value = 255;
  FILE* fp = fopen(file_path.c_str(), "w");
  fprintf(fp, "P3\n%d %d\n%d\n", width, height, max_rgb_value);
  for (size_t i = 0; i < height; i++) {
    for (size_t j = 0; j < width; j++) {
      auto& data = pixels[i][j];
      fprintf(fp, "%4d%4d%4d ", data.R(), data.G(), data.B());
    }
    fprintf(fp, "\n");
  }

  fclose(fp);

  return 0;
}

#endif