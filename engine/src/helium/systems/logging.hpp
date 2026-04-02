#pragma once

#include "system_base.hpp"

#include <chrono>
#include <mutex>
#include <source_location>

#include "utils/type_traits/type_name.hpp"


enum severity
{
    info,
    warning,
    error
};

namespace ne
{

class logging: public he::system_base
{
public:
    auto log(std::chrono::zoned_time<std::chrono::duration<std::chrono::system_clock::rep, std::chrono::system_clock::period>> time,
             severity severity,
             std::string_view tag,
             std::string_view message,
             const std::source_location& location) -> void;

    auto set_severity_threshold(severity severity) -> void;

protected:
    auto tick(double dt) -> void override;

private:
    std::vector<std::string> _queue;

    severity _threshold{ severity::info };

    std::mutex _mutex;
};

template <typename... ts>
struct log
{
    log(severity severity,
        std::string_view tag,
        std::string_view format,
        ts&&... args,
        const std::source_location& location = std::source_location::current())
    {
        he::world::get_system<logging>().log(
            std::chrono::zoned_time{ std::chrono::current_zone(), std::chrono::system_clock::now() },
            severity,
            tag,
            std::vformat(format, std::make_format_args(args...)),
            location);
    }
};

template <typename... ts>
log(severity severity, std::string_view tag, std::string_view, ts&&...) -> log<ts...>;
}
