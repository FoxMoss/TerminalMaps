#include "tiles.hpp"
#include <cmath>
#include <cstddef>
#include <curses.h>
#include <string.h>
#include <tuple>
#include <utility>

void mvaddstr_nowrap(int y, int x, char *str,
                     PrecomputedTileRender *tile_render) {
  // this is naive, in future change position of null
  size_t len = strlen((const char *)str);

  bool hit = true;
  int offset_y = 0;
  int offset_x = 0;
  uint step = 1;
  uint rotation = 0;
  if (tile_render != NULL) {
    while (hit) {
      switch (rotation % 2) {
      case 0:
        offset_y = step;
        break;
      case 1:
        offset_y = -step;
        break;
      }
      hit = false;
      for (int iter = 0; str[iter] != '\0'; iter++) {
        char got = tile_render->get_pos(y - offset_y,
                                        (x + iter - (int)(len / 2)) - offset_x);
        if (got != 0 && got != '#') {
          hit = true;
        }
      }
      step++;
      rotation++;
    }
  }
  for (int iter = 0; str[iter] != '\0'; iter++) {
    if (iter + x - (int)(len / 2) > COLS && tile_render == NULL)
      return;

    if (tile_render == NULL)
      mvaddch(y - offset_y, x + iter - (int)(len / 2), str[iter]);
    else
      tile_render->draw_char(y - offset_y, x + iter - (int)(len / 2) - offset_x,
                             str[iter]);
  }
}

// https://gist.github.com/JamesPhillipsUK/fae5f5bf1e62fdf4118ed37dbbc258d2
void draw_line(std::pair<int, int> point_1, std::pair<int, int> point_2,
               PrecomputedTileRender *tile_render = NULL) {
  int dx, sx, dy, sy, gradient_error, gradient_error_two;

  dx = abs(point_2.first - point_1.first);
  sx = point_1.first < point_2.first ? 1 : -1;
  dy = abs(point_2.second - point_1.second);
  sy = point_1.second < point_2.second ? 1 : -1;

  gradient_error = (dx > dy ? dx : -dy) / 2;
  for (;;) {
    if (tile_render == NULL) {
      mvaddch(point_1.second, point_1.first, '#');
    } else {
      tile_render->draw_char(point_1.second, point_1.first, '#');
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
