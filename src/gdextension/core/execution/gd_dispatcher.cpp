#include "gd_dispatcher.h"

#include <godot_cpp/classes/worker_thread_pool.hpp>


namespace he
{

auto gd_dispatcher::dispatch(std::function<void()> work) -> void
{
    auto* work_ptr{ new std::function<void()>{ std::move(work) } };

    godot::WorkerThreadPool::get_singleton()->add_native_task(
        [] (void* userdata)
        {
            auto* fn{ static_cast<std::function<void()>*>(userdata) };

            (*fn)();

            delete fn;
        },
        work_ptr);
}

}
