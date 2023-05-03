#pragma once

#include "vec3.hpp"

struct Item
{
    enum class Type
    {
        X, O, None
    };

    Item();

    [[nodiscard]] bool none() const;

    vec3 position;
    Type type;
};