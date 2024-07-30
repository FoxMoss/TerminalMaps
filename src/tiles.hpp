#pragma once

#include "vector_tile.pb.h"
#include <cstdlib>
#include <curses.h>
#include <sys/types.h>
#include <unordered_map>
#include <utility>

class Viewer {
public:
  Viewer() {}
  ~Viewer() {}

  std::pair<float, float> global_cursor{0, 0};
  float global_scale = 10;
  uint zoom = 1;

private:
};

struct TileInfo {
  int x;
  int y;
  int zoom;
  bool operator==(const TileInfo &other) {
    return (other.x == y && other.y == y && other.zoom == zoom);
  }
};

class PrecomputedTileRender {
public:
  PrecomputedTileRender(TileInfo info) {
    tile_info = info;
    buffer = (char *)malloc(COLS * LINES);
  }
  ~PrecomputedTileRender() { free(buffer); }

  void draw_char(int y, int x, char c) {}

private:
  TileInfo tile_info;
  char *buffer;
};

#define POS(x, y) x + y *COLS

typedef void (*FeatureHandler)(vector_tile::Tile::Feature, Viewer *,
                               vector_tile::Tile::Layer *, TileInfo *);

std::ifstream download_tile(std::string base_url, TileInfo tile,
                            std::string save_path);
std::vector<std::pair<std::pair<int, int>, std::pair<uint, uint>>>
get_drawn_tiles(Viewer *view, uint zoom, bool debug);
void draw_layer(vector_tile::Tile tile, std::string layer_name,
                Viewer *global_view, FeatureHandler handler,
                TileInfo tile_info = {0, 0, 0});

void outline_handler(vector_tile::Tile::Feature feature, Viewer *global_view,
                     vector_tile::Tile::Layer *, TileInfo *tile_info);
void text_handler(vector_tile::Tile::Feature feature, Viewer *global_view,
                  vector_tile::Tile::Layer *layer, TileInfo *tile_info);
