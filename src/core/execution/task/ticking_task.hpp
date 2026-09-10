#pragma once

#include "core/execution/task/task_base.hpp"

#include <optional>
#include <stop_token>


namespace he::exec
{

class ticking_task final: public task_base
{
public:
    explicit ticking_task(ticking_definition definition);

    auto tick() -> void override;
    auto get_result() -> std::optional<task_result> override;
    auto cancel() -> void override;

private:
    ticking_definition definition;
    std::stop_source stop_source;
    std::optional<task_result> _result;
};

}
