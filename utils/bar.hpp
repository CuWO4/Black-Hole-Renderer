#ifndef BAR_HPP__
#define BAR_HPP__

#include <cstdio>

class Bar {
public:

  Bar(int total) 
    : current(0), last(0), 
      per_cell((total + total_characters - 1) / total_characters) {
    putchar('[');
    for (int i = 0; i < total_characters; i++) {
      putchar(' ');
    }
    putchar(']');
    printf("\x1b[2G");
  }

  void step() {
    current++;
    if (current > last * per_cell) {
      putchar('#');
      last++;
    }
  }

  ~Bar() {
    putchar(']');
    putchar('\n');
  }

private:
  static constexpr int total_characters = 50;

  int current;
  int last;
  int per_cell;
};

#endif