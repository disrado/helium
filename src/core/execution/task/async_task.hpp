#pragma once

#include "core/execution/dispatcher.hpp"
#include "core/execution/task/task_base.hpp"

#include <future>
#include <memory>
#include <optional>
#include <stop_token>


namespace he::exec
{

class async_task final: public task_base
{
public:
    async_task(task_definition definition, std::shared_ptr<dispatcher> dispatcher_ptr);

    auto tick() -> void override;
    auto get_status() -> std::optional<task_result> override;
    auto cancel() -> void override;

private:
    std::future<task_result> _future;
    std::stop_source stop_source;
};

}
