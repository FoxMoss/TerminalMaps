#include "tiles.hpp"
#include <cmath>
#include <curses.h>
#include <string.h>
#include <tuple>
#include <utility>

void mvaddstr_nowrap(int y, int x, char *str) {
  // this is naive, in future change position of null
  size_t len = strlen((const char *)str);

  for (int iter = 0; str[iter] != '\0'; iter++) {
    if (iter + x - (int)(len / 2) > COLS) {
      return;
    }

    mvaddch(y, x + iter - (int)(len / 2), str[iter]);
  }
}

// https://gist.github.com/JamesPhillipsUK/fae5f5bf1e62fdf4118ed37dbbc258d2
void draw_line(std::pair<int, int> point_1, std::pair<int, int> point_2,
               char *buffer) {
  int dx, sx, dy, sy, gradient_error, gradient_error_two;

  if (!((point_1.first > -COLS && point_1.first < COLS * 2 &&
         point_1.second > -LINES && point_1.second < LINES * 2) &&
        (point_2.first > -COLS && point_2.first < COLS * 2 &&
         point_2.second > -LINES && point_2.second < LINES * 2))) {
    return;
  }

  dx = abs(point_2.first - point_1.first);
  sx = point_1.first < point_2.first ? 1 : -1;
  dy = abs(point_2.second - point_1.second);
  sy = point_1.second < point_2.second ? 1 : -1;

  gradient_error = (dx > dy ? dx : -dy) / 2;
  for (;;) {
    if (buffer == NULL) {
      mvaddch(point_1.second, point_1.first, '.');
    } else {
    }
    if (point_1.first == point_2.first && point_1.second == point_2.second)
      break;

    gradient_error_two = gradient_error;
    if (gradient_error_two > -dx) {
      gradient_error -= dy;
      point_1.first += sx;
    }
    if (gradient_error_two < dy) {
      gradient_error += dx;
      point_1.second += sy;
    }
  }
}
