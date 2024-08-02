#pragma once
#include "tiles.hpp"
#include <cstddef>
#include <map>

void draw_line(std::pair<int, int> point_1, std::pair<int, int> point_2,
               PrecomputedTileRender *tile_render = NULL);
void mvaddstr_nowrap(int y, int x, char *, PrecomputedTileRender *tile_render);
