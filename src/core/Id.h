#pragma once

#include <cstdint>

struct WidgetId
{
    std::uint64_t value = 0;

    friend bool operator==(WidgetId a, WidgetId b)
    {
        return a.value == b.value;
    }

    friend bool operator!=(WidgetId a, WidgetId b)
    {
        return !(a == b);
    }
};
