#pragma once

#include <algorithm>


namespace he
{

template <auto Size>
struct string_literal final
{
public:
    constexpr string_literal(const char (&data)[Size])
    {
        std::copy(data, data + Size, value);
    }

public:
    char value[Size];
};

}
