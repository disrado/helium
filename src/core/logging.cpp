#include "core/logging.hpp"

#include <cassert>


namespace he
{

auto logging::create() -> std::unique_ptr<logging>
{
    return std::make_unique<logging>();
}


logging::logging()
{
    const auto trap{
        [] (const log_entry&)
        {
            assert(false && "severity dispatcher not set");
        } };

    _dispatchers[info] = trap;
    _dispatchers[warning] = trap;
    _dispatchers[error] = trap;
}


auto logging::log(std::chrono::zoned_time<std::chrono::duration<std::chrono::system_clock::rep, std::chrono::system_clock::period>> time,
                  severity severity,
                  std::string_view tag,
                  std::string_view message,
                  const std::source_location& location) -> void
{
    if (severity < _threshold)
    {
        return;
    }

    _dispatchers[severity](
        log_entry{
            .time{ time },
            .tag{ tag },
            .message{ message },
            .location{ location }
        });
}


auto logging::set_severity_threshold(severity severity) -> void
{
    _threshold = severity;
}


auto logging::set_severity_dispatcher(severity severity, dispatcher_t dispatcher) -> void
{
    _dispatchers[severity] = std::move(dispatcher);
}

}
