#ifndef BAR_HPP__
#define BAR_HPP__

#include <algorithm>
#include <cstdio>

class Bar {
public:

  Bar(int total) 
    : total(std::max(total, 0)), current(0), rendered(0) {
    putchar('[');
    for (int i = 0; i < total_characters; i++) {
      putchar(' ');
    }
    putchar(']');
    printf("\x1b[2G");
  }

  void step() {
    if (current >= total) {
      return;
    }

    current++;
    const int target = total == 0
      ? total_characters
      : (current * total_characters) / total;

    while (rendered < target) {
      putchar('#');
      rendered++;
    }

    std::fflush(stdout);
  }

  ~Bar() {
    std::printf("\x1b[%dG\n", total_characters + 3);
  }

private:
  static constexpr int total_characters = 50;

  int total;
  int current;
  int rendered;
};

#endif