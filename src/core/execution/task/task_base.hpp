#pragma once

#include "core/execution/defs.hpp"

#include <optional>


namespace he::exec
{

class task_base
{
public:
    virtual ~task_base() = default;

    virtual auto tick() -> void = 0;
    virtual auto get_status() -> std::optional<task_result> = 0;   // pure query, safe anytime
    virtual auto cancel() -> void = 0;
};

}
