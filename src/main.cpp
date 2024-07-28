
#include "tiles.hpp"
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
  vector_tile::Tile tile2;
  std::ifstream file2("1.pbf");
  if (!tile2.ParseFromIstream(&file2)) {
    printf("Not happening\n");
    exit(0);
  }

  for (auto layer : tile.layers()) {
    printf("Layer: %s\n", layer.name().c_str());
    // if (layer.name() == "place") {
    //   for (auto keys : layer.keys()) {
    //     printf("Keys: %s\n", keys.c_str());
    //   }
    // }
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
    init_pair(4, COLOR_BLACK, COLOR_BLUE);
    init_pair(5, COLOR_CYAN, COLOR_BLACK);
    init_pair(6, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(7, COLOR_WHITE, COLOR_BLACK);
    init_pair(8, COLOR_RED, COLOR_BLACK);
  }

  curs_set(0);

  Viewer global_view;
  cbreak();
  for (;;) {
    clear();

    attrset(COLOR_PAIR(7));
    // draw_layer(tile, "boundry", &global_view, outline_handler, {0, 0, 1});
    // draw_layer(tile2, "boundry", &global_view, outline_handler, {1, 0, 1});

    attrset(COLOR_PAIR(4));
    draw_layer(tile, "water", &global_view, outline_handler, {0, 0, 1});
    draw_layer(tile2, "water", &global_view, outline_handler, {2, 1, 2});
    attrset(COLOR_PAIR(2));
    get_drawn_tiles(&global_view, global_view.zoom, true);
    // draw_layer(tile, "place", &global_view, text_handler, {0, 0, 1});

    // draw_layer(tile2, "place", &global_view, text_handler, {1, 0, 1});

    mvprintw(LINES - 1, 0, "[N] (%f, %f) scale %f zoom %i\n",
             global_view.global_cursor.first, global_view.global_cursor.second,
             global_view.global_scale, global_view.zoom);

    attrset(COLOR_PAIR(1));
    auto key = getch();
    float step = 5 * global_view.global_scale;
    if (key == 'l') {
      global_view.global_cursor.first -= step;
    }
    if (key == 'h') {
      global_view.global_cursor.first += step;
    }
    if (key == 'k') {
      global_view.global_cursor.second += step;
    }
    if (key == 'j') {
      global_view.global_cursor.second -= step;
    }
    if (key == 'w' && global_view.zoom < 22) {
      global_view.global_scale /= 2;
      global_view.zoom++;
    }
    if (key == 'b' && global_view.zoom > 0) {
      global_view.global_scale *= 2;
      global_view.zoom--;
    }
  }

  finish(0);
}

static void finish(int) {
  endwin();
  nocbreak();

  exit(0);
}
