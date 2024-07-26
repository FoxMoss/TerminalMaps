
#include "vector_tile.pb.h"

#include "drawing.hpp"
#include <cstdint>
#include <cstdio>
#include <curses.h>
#include <fstream>
#include <math.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <utility>
#include <vector>

static void finish(int sig);

int main() {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  vector_tile::Tile tile;
  std::ifstream file("0.pbf");
  if (!tile.ParseFromIstream(&file)) {
    printf("Not happening\n");
    exit(0);
  }
  for (auto layer : tile.layers()) {
    printf("Layer: %s\n", layer.name().c_str());
    if (layer.name() == "place") {
      for (auto keys : layer.keys()) {
        printf("Keys: %s\n", keys.c_str());
      }
    }
  }
  signal(SIGINT, finish);

  struct winsize term_window;
  ioctl(0, TIOCGWINSZ, &term_window);

  initscr();
  keypad(stdscr, TRUE);
  nonl();
  cbreak();
  echo();

  if (has_colors()) {
    start_color();

    init_pair(1, COLOR_BLACK, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);
    init_pair(4, COLOR_BLUE, COLOR_BLACK);
    init_pair(5, COLOR_CYAN, COLOR_BLACK);
    init_pair(6, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(7, COLOR_WHITE, COLOR_BLACK);
    init_pair(8, COLOR_RED, COLOR_BLACK);
  }

  curs_set(0);

  std::pair<float, float> global_cursor(0, 0);
  float global_scale = 10;
  for (;;) {
    clear();
    attrset(COLOR_PAIR(4));

    mvprintw(0, 0, "%s", (tile.layers().begin() + 3)->name().c_str());
    for (auto feature : (tile.layers().begin() + 3)->features()) {
      if (feature.type() != 3) {
        mvprintw(1, 0, "%i", feature.type());
        continue;
      }
      uint32_t current_id = 0;
      uint32_t current_count = 0;
      uint32_t param_left = 0;
      bool pair_filled = false;
      std::pair<int, int> pair;
      std::pair<int, int> cursor = global_cursor;
      std::vector<std::pair<int, int>> params;
      for (auto outline : feature.geometry()) {
        if (param_left == 0) {
          if (current_id != 0) {
            // handle last
            std::pair<int, int> last_param(0, 0);
            for (auto param : params) {
              if (current_id == 1) {
                cursor.first += param.first;
                cursor.second += param.second;
              }

              if (current_id == 2) {
                float pos_x = cursor.first += param.first;
                float pos_y = cursor.second += param.second;
                std::pair<int, int> proccessed_param(
                    roundf(((float)COLS / 2) + pos_x / (global_scale)),
                    roundf(((float)LINES / 2) + pos_y / (global_scale * 2)));

                if (last_param != std::pair<int, int>(0, 0)) {
                  draw_line(proccessed_param, last_param);
                }
                last_param = proccessed_param;
              }
            }
          }

          params.clear();

          // push new command
          current_id = outline & 0x7;
          current_count = outline >> 3;
          if (current_id == 1 || current_id == 2) {
            param_left = 2 * current_count;
          }
          continue;
        }

        int value = ((outline >> 1) ^ (-(outline & 1)));
        if (!pair_filled) {
          pair.first = value;
          pair_filled = true;
        } else {
          pair.second = value;
          params.push_back(pair);
          pair_filled = false;
        }

        param_left--;
      }
    }
    attrset(COLOR_PAIR(1));
    auto key = getch();
    float step = 5 * global_scale;
    if (key == 'l') {
      global_cursor.first -= step;
    }
    if (key == 'h') {
      global_cursor.first += step;
    }
    if (key == 'k') {
      global_cursor.second += step;
    }
    if (key == 'j') {
      global_cursor.second -= step;
    }
    if (key == 'w') {
      global_scale /= 2;
    }
    if (key == 'b') {
      global_scale *= 2;
    }
  }

  finish(0);
}

static void finish(int) {
  endwin();

  exit(0);
}
