#include "thread_dispatcher.hpp"

#include <thread>


namespace he::exec
{

auto thread_dispatcher::dispatch(std::function<void()> work) -> void
{
    std::thread{ std::move(work) }.detach();
}

}
