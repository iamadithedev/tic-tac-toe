#include "item.hpp"

Item::Item()
    : type { Type::None }
{
}

bool Item::none() const
{
    return type == Type::None;
}

void Item::reset()
{
    type = Type::None;
}