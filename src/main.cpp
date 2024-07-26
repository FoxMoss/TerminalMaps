#include "vector_tile.pb.h"
#include <cstdint>
#include <cstdio>
#include <curses.h>
#include <fstream>
#include <signal.h>
#include <stdlib.h>
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
  printf("Feature geo type %i\n",
         tile.layers().begin()->features().begin()->type());
  signal(SIGINT, finish);

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

  uint32_t current_id = 0;
  uint32_t current_count = 0;
  uint32_t param_left = 0;
  bool pair_filled = false;
  std::pair<int, int> pair;
  std::pair<int, int> cursor(10, 10);
  std::vector<std::pair<int, int>> params;
  for (auto outline : tile.layers().begin()->features().begin()->geometry()) {
    if (param_left == 0) {
      if (current_id != 0) {
        // handle last
        // printw("comand %i times %i params ", current_id, current_count);
        for (auto param : params) {
          if (current_id == 1) {
            // cursor = param;
          }
          if (current_id == 2) {
            cursor.first += param.first;
            cursor.second += param.second;
            mvaddch(cursor.first, cursor.second, '.');
          }
          // printw(" (%i, %i)", param.first, param.second);
        }
        // printw("\n");
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

  curs_set(0);
  int num = 0;
  for (;;) {
    attrset(COLOR_PAIR(1));
    auto key = getch();
    num++;
  }

  finish(0);
}

static void finish(int) {
  endwin();

  exit(0);
}
