#pragma once
#include <cstddef>
#include <map>

void draw_line(std::pair<int, int> point_1, std::pair<int, int> point_2,
               char *buffer = NULL);
void mvaddstr_nowrap(int y, int x, char *);
