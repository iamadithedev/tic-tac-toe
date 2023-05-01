#pragma once

#include "grid.hpp"
#include "item.hpp"

class Board final : public Grid<Item, 3, 3>
{
public:
    void init(float offset);
};