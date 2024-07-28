#include "tiles.hpp"
#include "drawing.hpp"
#include "vector_tile.pb.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <curses.h>
#include <math.h>
#include <utility>
#include <vector>

std::pair<int, int> get_screen_coords(std::pair<float, float> world_pos,
                                      Viewer *view, TileInfo *tile_info) {
  float tile_size = 4096 / std::pow(2, tile_info->zoom + 1);
  return std::pair<int, int>(
      roundf(((((world_pos.first / std::pow(tile_info->zoom + 1, 2)) +
                view->global_cursor.first) +
               (tile_size * tile_info->x))) /
             (view->global_scale)),
      roundf((((world_pos.second / std::pow(tile_info->zoom + 1, 2)) +
               view->global_cursor.second) +
              (tile_size * tile_info->y)) /
             (view->global_scale * 2)));
}

void outline_handler(vector_tile::Tile::Feature feature, Viewer *global_view,
                     vector_tile::Tile::Layer *, TileInfo *tile_info) {
  uint32_t current_id = 0;
  uint32_t current_count = 0;
  uint32_t param_left = 0;
  bool pair_filled = false;
  std::pair<int, int> pair;
  std::pair<int, int> cursor{0, 0};
  std::vector<std::pair<int, int>> params;
  std::vector<std::vector<std::pair<int, int>>> shapes;

  for (auto outline : feature.geometry()) {
    if (param_left <= 0) {
      if (current_id != 0) {
        // handle last
        bool start = true;
        std::pair<int, int> last_param(0, 0);
        std::pair<int, int> first_param(0, 0);
        for (auto param : params) {
          if (current_id == 1) {
            cursor.first += param.first;
            cursor.second += param.second;
          }

          if (current_id == 2) {
            float pos_x = cursor.first += param.first;
            float pos_y = cursor.second += param.second;

            if (!start) {
              if (!(pos_x <= 0 || pos_x >= 4096 || pos_y <= 0 ||
                    pos_y >= 4096 || last_param.first <= 0 ||
                    last_param.first >= 4096 || last_param.second <= 0 ||
                    last_param.second >= 4096)) {
                draw_line(
                    get_screen_coords({pos_x, pos_y}, global_view, tile_info),
                    get_screen_coords({last_param.first, last_param.second},
                                      global_view, tile_info));
                // auto pos =
                //     get_screen_coords({pos_x, pos_y}, global_view,
                //     tile_info);
                // mvprintw(pos.second, pos.first, "(%f, %f)", pos_x, pos_y);
              }
            } else {
              start = false;
              first_param = {pos_x, pos_y};
            }
            last_param = {pos_x, pos_y};
          }
        }
        // close shape
        if (!(first_param.first <= 0 || first_param.first >= 4096 ||
              first_param.second <= 0 || first_param.second >= 4096 ||
              last_param.first <= 0 || last_param.first >= 4096 ||
              last_param.second <= 0 || last_param.second >= 4096)) {
          draw_line(first_param, last_param);
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
void text_handler(vector_tile::Tile::Feature feature, Viewer *global_view,
                  vector_tile::Tile::Layer *layer, TileInfo *tile_info) {
  bool pair_filled = false;
  std::pair<uint, uint> pair;
  std::vector<std::pair<uint, uint>> raw_pairs;
  for (auto tag : feature.tags()) {
    if (!pair_filled) {
      pair.first = tag;
      pair_filled = true;
    } else {
      pair.second = tag;
      raw_pairs.push_back(pair);
      pair_filled = false;
    }
  }
  std::map<std::string, std::string> keyval;
  for (auto pair : raw_pairs) {
    auto key = layer->keys()[pair.first];
    if (!layer->values()[pair.second].has_string_value())
      continue;
    auto val = layer->values()[pair.second].string_value();
    keyval[key] = val;
  }

  int pos_x = ((feature.geometry(1) >> 1) ^ (-(feature.geometry(1) & 1)));
  int pos_y = ((feature.geometry(2) >> 1) ^ (-(feature.geometry(2) & 1)));

  auto proccessed_param =
      get_screen_coords(std::pair<float, float>{(float)pos_x, (float)pos_y},
                        global_view, tile_info);
  if (!(proccessed_param.first > 0 && proccessed_param.first < COLS &&
        proccessed_param.second > 0 && proccessed_param.second < LINES)) {
    return;
  }

  if (keyval.find("name:en") != keyval.end()) {
    mvaddstr_nowrap(proccessed_param.second, proccessed_param.first,
                    (char *)keyval["name:en"].c_str());
  }
}

void draw_layer(vector_tile::Tile tile, std::string layer_name,
                Viewer *global_view, FeatureHandler handler,
                TileInfo tile_info) {
  vector_tile::Tile::Layer layer_copy;
  bool found = false;
  for (auto layer : tile.layers()) {
    if (layer.name() == layer_name) {
      layer_copy = layer;
      found = true;
    }
  }
  if (!found) {
    return;
  }
  for (auto feature : layer_copy.features()) {
    handler(feature, global_view, &layer_copy, &tile_info);
  }
}

void get_drawn_tiles(Viewer *view, uint zoom, bool debug) {
  float tile_size = 4096 / std::pow(2, zoom + 1);
  int tile_mod = std::pow(2, zoom);
  int start_tile_x = floor(-view->global_cursor.first / tile_size);
  int start_tile_y = floor(-view->global_cursor.second / tile_size);

  std::vector<std::pair<int, int>> tiles;
  for (int x = start_tile_x; x < start_tile_x + 3; x++) {
    for (int y = std::clamp(start_tile_y, 0, tile_mod);
         y < std::clamp(start_tile_y + 3, 0, tile_mod); y++) {
      tiles.push_back({x, y});
    }
  }

  if (debug) {
    TileInfo null_tile{0, 0, 0};
    for (auto tile : tiles) {
      auto screen = get_screen_coords(
          {tile.first * tile_size, tile.second * tile_size}, view, &null_tile);
      mvprintw(screen.second, screen.first, "tile %i/%i/%i", zoom,
               abs(tile.first % tile_mod), abs(tile.second % tile_mod));
    }
  }
}
