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
