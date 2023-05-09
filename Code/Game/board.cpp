#include "board.hpp"

void Board::init(float offset)
{
    float y = offset;

    for (int32_t row = 0; row < rows(); row++)
    {
        float x = -offset;

        for (int32_t column = 0; column < columns(); column++)
        {
            item_at(row, column).position = { x, y, 0.0f };
            x += offset;
        }

        y -= offset;
    }
}

bool Board::check_row(int32_t row, Item::Type type)
{
    bool result = item_at(row, 0).type == type &&
                  item_at(row, 1).type == type &&
                  item_at(row, 2).type == type;
    if (result)
    {
        std::cout << "row win\n";
    }

    return result;
}

bool Board::check_column(int32_t column, Item::Type type)
{
    bool result = item_at(0, column).type == type &&
                  item_at(1, column).type == type &&
                  item_at(2, column).type == type;
    if (result)
    {
        std::cout << "column win\n";
    }

    return result;
}

bool Board::check_diagonals(Item::Type type)
{
    bool result = item_at(0, 0).type == type &&
                  item_at(1, 1).type == type &&
                  item_at(2, 2).type == type;
    if (result)
    {
        std::cout << "diagonals win\n";
        return true;
    }

    result = item_at(0, 2).type == type &&
             item_at(1, 1).type == type &&
             item_at(2, 0).type == type;

    if (result)
    {
        std::cout << "diagonals win\n";
    }

    return result;
}

void Board::reset()
{
    for (int32_t row = 0; row < rows(); row++)
    {
        for (int32_t column = 0; column < columns(); column++)
        {
            item_at(row, column).reset();
        }
    }

    std::cout << "board reset\n";
}

bool Board::check_win(int32_t row, int32_t column, Item::Type type)
{
    return check_diagonals(type) || check_row(row, type) || check_column(column, type);
}
