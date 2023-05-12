#pragma once

#include "grid.hpp"
#include "item.hpp"

class Board final : public Grid<Item, 3, 3>
{
public:
    void init();
    void reset();

    bool check_win(int32_t row, int32_t column, Item::Type type);

private:
    bool check_row(int32_t row, Item::Type type);
    bool check_column(int32_t column, Item::Type type);

    bool check_diagonals(Item::Type type);
};