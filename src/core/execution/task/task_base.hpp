#pragma once

#include "core/execution/defs.hpp"

#include <optional>


namespace he::exec
{

class task_base
{
public:
    virtual ~task_base() = default;

    virtual auto tick() -> void;
    virtual auto get_result() -> std::optional<task_result> = 0;
    virtual auto cancel() -> void;
};

}
