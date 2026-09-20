#pragma once

#include "core/singleton.hpp"

#include <chrono>
#include <format>
#include <functional>
#include <map>
#include <memory>
#include <source_location>
#include <string_view>


enum severity
{
    info,
    warning,
    error
};

namespace he
{

struct log_entry final
{
    std::chrono::zoned_time<std::chrono::duration<std::chrono::system_clock::rep, std::chrono::system_clock::period>> time;
    std::string_view tag;
    std::string_view message;
    std::source_location location;
};

class logging final: public he::singleton<logging>
{
public:
    using dispatcher_t = std::function<void(const log_entry&)>;

public:
    static auto create() -> std::unique_ptr<logging>;

    logging();

    auto log(std::chrono::zoned_time<std::chrono::duration<std::chrono::system_clock::rep, std::chrono::system_clock::period>> time,
             severity severity,
             std::string_view tag,
             std::string_view message,
             const std::source_location& location) -> void;

    auto set_severity_threshold(severity severity) -> void;

    auto set_severity_dispatcher(severity severity, dispatcher_t dispatcher) -> void;

private:
    severity _threshold{ severity::info };

    std::map<severity, dispatcher_t> _dispatchers;
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
        he::logging::instance().log(
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
