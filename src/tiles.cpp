#include "tiles.hpp"
#include "drawing.hpp"
#include "vector_tile.pb.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <curl/curl.h>
#include <curl/easy.h>
#include <curses.h>
#include <fstream>
#include <istream>
#include <math.h>
#include <string>
#include <utility>
#include <vector>

std::pair<int, int> get_scaled_coords(std::pair<float, float> world_pos,
                                      Viewer *view, TileInfo *tile_info) {
  float tile_size = (float)4096 / std::pow(2, tile_info->zoom);
  int tile_count = std::pow(2, tile_info->zoom);
  return std::pair<int, int>(
      roundf(((((world_pos.first / tile_count)) + (tile_size * tile_info->x))) /
             (view->global_scale)),
      roundf((((world_pos.second / tile_count)) + (tile_size * tile_info->y)) /
             (view->global_scale * 2)));
}

void outline_handler(vector_tile::Tile::Feature feature, Viewer *global_view,
                     vector_tile::Tile::Layer *layer, TileInfo *tile_info,
                     PrecomputedTileRender *tile_render) {
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
                    get_scaled_coords({pos_x, pos_y}, global_view, tile_info),
                    get_scaled_coords({last_param.first, last_param.second},
                                      global_view, tile_info),
                    tile_render);
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
          draw_line(first_param, last_param, tile_render);
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
                  vector_tile::Tile::Layer *layer, TileInfo *tile_info,
                  PrecomputedTileRender *tile_render) {
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
    std::string val = "";
    if (layer->values()[pair.second].has_string_value()) {
      val = layer->values()[pair.second].string_value();
    }
    if (layer->values()[pair.second].has_int_value()) {
      val = std::to_string(layer->values()[pair.second].int_value());
    }
    if (layer->values()[pair.second].has_uint_value()) {
      val = std::to_string(layer->values()[pair.second].uint_value());
    }
    if (layer->values()[pair.second].has_sint_value()) {
      val = std::to_string(layer->values()[pair.second].sint_value());
    }
    keyval[key] = val;
  }

  int pos_x = ((feature.geometry(1) >> 1) ^ (-(feature.geometry(1) & 1)));
  int pos_y = ((feature.geometry(2) >> 1) ^ (-(feature.geometry(2) & 1)));

  auto proccessed_param =
      get_scaled_coords(std::pair<float, float>{(float)pos_x, (float)pos_y},
                        global_view, tile_info);
  if (std::stoi(keyval["rank"]) > std::max((int)global_view->zoom, 1)) {
    return;
  }
  if (keyval.find("name:en") != keyval.end()) {
    mvaddstr_nowrap(proccessed_param.second, proccessed_param.first,
                    (char *)keyval["name:en"].c_str(), tile_render);
  }
}

void draw_layer(vector_tile::Tile tile, std::string layer_name,
                Viewer *global_view, FeatureHandler handler, TileInfo tile_info,
                PrecomputedTileRender *tile_render) {
  vector_tile::Tile::Layer layer_copy;
  bool found = false;
  for (auto layer : tile.layers()) {
    if (layer.name() == layer_name) {
      layer_copy = layer;
      found = true;
      break;
    }
  }
  if (!found) {
    return;
  }
  for (auto feature : layer_copy.features()) {
    handler(feature, global_view, &layer_copy, &tile_info, tile_render);
  }
}

std::vector<std::pair<std::pair<int, int>, std::pair<uint, uint>>>
get_drawn_tiles(Viewer *view, uint zoom, bool debug) {
  float tile_size = 4096 / std::pow(2, zoom);
  int tile_mod = std::pow(2, zoom);
  int start_tile_x = floor(-view->global_cursor.first / tile_size);
  int start_tile_y = floor(-view->global_cursor.second / tile_size * 2);

  std::vector<std::pair<std::pair<int, int>, std::pair<uint, uint>>> tiles;
  for (int x = std::clamp(start_tile_x, 0, tile_mod);
       x < std::clamp(start_tile_x + 2, 0, tile_mod); x++) {
    for (int y = std::clamp(start_tile_y, 0, tile_mod);
         y < std::clamp(start_tile_y + 2, 0, tile_mod); y++) {
      tiles.push_back({{x, y}, {abs(x % tile_mod), abs(y % tile_mod)}});
    }
  }

  if (debug) {
    for (auto tile : tiles) {
      TileInfo got_tile{tile.first.first, tile.first.second, (int)zoom};
      auto screen = get_scaled_coords({0, 0}, view, &got_tile);
      int real_x = screen.first +
                   roundf((view->global_cursor.first / view->global_scale));
      int real_y = screen.second +
                   roundf((view->global_cursor.second / view->global_scale));
      mvprintw(real_y, real_x, "tile %i/%i/%i sized %f", zoom,
               tile.second.first, tile.second.second, tile_size);
    }
  }
  return tiles;
}

std::ifstream download_tile(std::string base_url, TileInfo tile,
                            std::string save_path) {
  auto curl = curl_easy_init();
  std::string tile_url = base_url + std::to_string(tile.zoom) + "/" +
                         std::to_string(tile.x) + "/" + std::to_string(tile.y) +
                         ".mvt";
  std::string save_file = save_path + "/" + std::to_string(tile.zoom) + "." +
                          std::to_string(tile.x) + "." +
                          std::to_string(tile.y) + ".mvt";

  curl_easy_setopt(curl, CURLOPT_URL, tile_url.c_str());
  FILE *fp = fopen(save_file.c_str(), "w");
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fwrite);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
  curl_easy_perform(curl);
  fclose(fp);
  curl_easy_cleanup(curl);
  mvprintw(LINES - 2, 0, "wrote file %s\n", save_file.c_str());
  return std::ifstream{save_file};
}
