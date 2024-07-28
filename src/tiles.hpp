#pragma once

#include "vector_tile.pb.h"
#include <sys/types.h>
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
  uint x;
  uint y;
  uint zoom;
  bool fill = false;
};

#define POS(x, y) x + y *COLS

typedef void (*FeatureHandler)(vector_tile::Tile::Feature, Viewer *,
                               vector_tile::Tile::Layer *, TileInfo *);

void get_drawn_tiles(Viewer *view, uint zoom, bool debug = false);
void draw_layer(vector_tile::Tile tile, std::string layer_name,
                Viewer *global_view, FeatureHandler handler,
                TileInfo tile_info = {0, 0, 0});

void outline_handler(vector_tile::Tile::Feature feature, Viewer *global_view,
                     vector_tile::Tile::Layer *, TileInfo *tile_info);
void text_handler(vector_tile::Tile::Feature feature, Viewer *global_view,
                  vector_tile::Tile::Layer *layer, TileInfo *tile_info);
