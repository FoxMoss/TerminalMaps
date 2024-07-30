
#include "tiles.hpp"
#include "vector_tile.pb.h"

#include "drawing.hpp"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <curses.h>
#include <filesystem>
#include <fstream>
#include <map>
#include <math.h>
#include <signal.h>
#include <stdlib.h>
#include <string>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <utility>
#include <vector>

#ifndef DEVICE
#define DEVICE CL_DEVICE_TYPE_DEFAULT
#endif

const std::string target_url = "https://d17gef4m69t9r4.cloudfront.net/planet/";
static void finish(int sig);

std::vector<std::pair<TileInfo, vector_tile::Tile *>> tile_map;
int main() {

  GOOGLE_PROTOBUF_VERIFY_VERSION;

  for (auto const &dir_entry : std::filesystem::directory_iterator{"."}) {
    if (dir_entry.path().extension() == ".mvt") {

      vector_tile::Tile *file_tile = new vector_tile::Tile;
      std::ifstream file(dir_entry.path());
      if (!file_tile->ParseFromIstream(&file)) {
        printf("Not happening\n");
        exit(0);
      }
      std::vector<uint> path_numbers;
      std::string current_num;
      for (auto c : dir_entry.path().filename().string()) {
        if (c == '.') {
          path_numbers.push_back(std::stoi(current_num));
          current_num = "";
          continue;
        }
        if (c == 'm') {
          break;
        }
        current_num.push_back(c);
      }
      tile_map.push_back(
          {{(int)path_numbers[1], (int)path_numbers[2], (int)path_numbers[0]},
           file_tile});
    }
  }

  signal(SIGINT, finish);

  struct winsize term_window;
  ioctl(0, TIOCGWINSZ, &term_window);

  initscr();
  keypad(stdscr, TRUE);
  nonl();
  cbreak();
  noecho();
  nodelay(stdscr, FALSE);

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

    attrset(COLOR_PAIR(2));
    auto needed_tiles = get_drawn_tiles(&global_view, global_view.zoom, false);

    uint tiles_found = 0;
    for (auto tile : needed_tiles) {
      // bunch of copying with to_draw, future optimization?
      vector_tile::Tile *to_draw;
      TileInfo tile_info{(int)tile.second.first, (int)tile.second.second,
                         (int)global_view.zoom};

      bool found = false;
      for (auto stored_tile : tile_map) {
        if ((stored_tile.first.x == tile_info.x &&
             stored_tile.first.y == tile_info.y &&
             stored_tile.first.zoom == tile_info.zoom)) {
          found = true;
          tiles_found++;
          to_draw = stored_tile.second;
          break;
        }
      }
      if (!found) {
        auto tile_file = download_tile(target_url, tile_info, ".");
        to_draw = new vector_tile::Tile;
        if (!to_draw->ParseFromIstream(&tile_file)) {
          continue;
        }
        mvprintw(0, 0, "tiles found, %i", tiles_found);
        tile_map.push_back({tile_info, to_draw});
      }

      attrset(COLOR_PAIR(4));
      draw_layer(*to_draw, "water", &global_view, outline_handler,
                 {tile.first.first, tile.first.second, (int)global_view.zoom});
      draw_layer(*to_draw, "place", &global_view, text_handler,
                 {tile.first.first, tile.first.second, (int)global_view.zoom});
    }

    attrset(COLOR_PAIR(7));
    // draw_layer(tile, "boundry", &global_view, outline_handler, {0, 0, 1});
    // draw_layer(tile2, "boundry", &global_view, outline_handler, {1, 0, 1});

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

  for (auto tile : tile_map) {
    delete tile.second;
  }

  exit(0);
}
