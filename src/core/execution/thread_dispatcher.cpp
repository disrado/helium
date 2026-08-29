#include "thread_dispatcher.hpp"


namespace he::exec
{

thread_dispatcher::thread_dispatcher()
    : _pool{ worker_count }
{
}


auto thread_dispatcher::dispatch(std::function<void()> work) -> void
{
    _pool.submit(std::move(work));
}

}
