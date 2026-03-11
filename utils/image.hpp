#ifndef IMAGE_HPP__
#define IMAGE_HPP__

#include "../include/vec3.hpp"

#include "../lib/stb_image_write.h"

#include <fstream>
#include <iomanip>
#include <vector>
#include <array>

template <size_t Height, size_t Width>
class Image {
public:
  static constexpr size_t width = Width;
  static constexpr size_t height = Height;

  std::array<Vec3, Width>& operator[](int i) { return pixels[i]; }
  std::array<Vec3, Width> operator[](int i) const { return pixels[i]; }

  std::vector<unsigned char> to_rgb_buffer() const;

  int save_as_ppm(std::string filepath);
  int save_as_png(std::string filepath, int compression = 6);
  int save_as_jpg(std::string filepath, int quality = 85);
  int save_as_bmp(std::string filepath);

private:
  std::array<std::array<Vec3, Width>, Height> pixels;

  std::vector<unsigned char> prepare_rgb_buffer() const {
    std::vector<unsigned char> buffer(Width * Height * 3);

    auto* ptr = buffer.data();
    for (const auto& row : pixels) {
      for (const auto& pixel : row) {
        *ptr++ = static_cast<unsigned char>(clamp(pixel.r) * 255.99);
        *ptr++ = static_cast<unsigned char>(clamp(pixel.g) * 255.99);
        *ptr++ = static_cast<unsigned char>(clamp(pixel.b) * 255.99);
      }
    }
    return buffer;
  }

  float clamp(float value) const {
    return (value < 0.0f) ? 0.0f : (value > 1.0f) ? 1.0f : value;
  }
};

template <size_t Height, size_t Width>
std::vector<unsigned char> Image<Height, Width>::to_rgb_buffer() const {
  return prepare_rgb_buffer();
}


template <size_t Height, size_t Width>
int Image<Height, Width>::save_as_ppm(std::string filepath) {
  constexpr int max_rgb_value = 255;

  std::ofstream out(filepath);
  if (!out) {
    return -1;
  }

  out << "P3" << std::endl
      << width << ' ' << height << std::endl
      << max_rgb_value << std::endl;

  for (auto& row : pixels) {
    for (auto& data : row) {
      out << std::setw(4) << data.R()
          << std::setw(4) << data.G()
          << std::setw(4) << data.B()
          << ' ';
    }
    out << std::endl;
  }

  std::cout << "saved to " << filepath << std::endl;

  return 0;
}

template <size_t Height, size_t Width>
int Image<Height, Width>:: save_as_png(std::string filepath, int compression) {
  auto buffer = prepare_rgb_buffer();
  if (!stbi_write_png(
    filepath.c_str(), Width, Height, 3,
    buffer.data(), Width * 3
  )) { return -1; }

  std::cout << "saved to " << filepath << std::endl;
  return 0;
}

template <size_t Height, size_t Width>
int Image<Height, Width>:: save_as_jpg(std::string filepath, int quality) {
  auto buffer = prepare_rgb_buffer();
  if (!stbi_write_jpg(
    filepath.c_str(), Width, Height, 3,
    buffer.data(), quality
  )) { return -1; }

  std::cout << "saved to " << filepath << std::endl;
  return 0;
}

template <size_t Height, size_t Width>
int Image<Height, Width>:: save_as_bmp(std::string filepath) {
  auto buffer = prepare_rgb_buffer();
  if (!stbi_write_bmp(
    filepath.c_str(), Width, Height, 3, buffer.data()
  )) { return -1; }

  std::cout << "saved to " << filepath << std::endl;
  return 0;
}

#endif