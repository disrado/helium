#pragma once

#include "core/execution/dispatcher.hpp"
#include "core/execution/thread_pool.hpp"


namespace he::exec
{

class thread_dispatcher final: public dispatcher
{
public:
    explicit thread_dispatcher();

    auto dispatch(std::function<void()> work) -> void override;

private:
    std::size_t worker_count{ 8 };

    thread_pool _pool;
};

}
