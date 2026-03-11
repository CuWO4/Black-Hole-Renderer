#include "include/constant.h"
#include "include/render/cuda_video_renderer.hpp"

#include <cstdio>
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
    << " -i - -an -c:v " << output::encoder
    << " -preset " << output::preset
    << " -rc " << output::rate_control
    << " -cq " << video::crf << " -b:v 0"
    << " -tag:v " << output::codec_tag << " -movflags +faststart"
    << " -pix_fmt " << output::pixel_format << ' '
    << shell_quote(out_path);

  return VIDEO_POPEN(command.str().c_str(), "w");
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

  FILE* video_pipe = open_video_pipe(out_path);
  if (!video_pipe) {
    std::cerr << "error: failed to start ffmpeg\n";
    return 1;
  }

  const int ok = render_video_cuda(video_pipe);

  if (VIDEO_PCLOSE(video_pipe) != 0) {
    std::cerr << "error: ffmpeg exited with non-zero status\n";
    return 1;
  }

  return ok ? 0 : 1;
}