#pragma once

#include <string_view>

#include "engine/core/system_base.hpp"
#include "engine/utils/string_literal.hpp"


namespace he
{


template <string_literal name>
class system: system_base
{
public:
    system()
    {
    }

public:
    virtual auto tick(double dt) -> void
    {
    }

public:
    static constexpr std::string_view _name{ name.value };
};

}
