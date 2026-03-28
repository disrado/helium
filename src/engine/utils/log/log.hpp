#pragma once

#include <chrono>
#include <format>
#include <functional>
#include <mutex>
#include <queue>
#include <source_location>


enum severity
{
    info,
    warning,
    error
};

namespace he
{

class log
{
public:
    explicit log(std::function<void(std::string&&)> output_call);

public:
    auto add_message(std::chrono::zoned_time<std::chrono::duration<std::chrono::system_clock::rep, std::chrono::system_clock::period>> time,
                     int32_t frame,
                     severity severity,
                     std::string_view tag,
                     std::string_view message,
                     const std::source_location& location) -> void;

    auto set_severity_threshold(severity severity) -> void;

    auto tick(double dt) -> void;

private:
    std::queue<std::string> _message_queue;

    std::function<void(std::string&&)> _output_call;

    severity _threshold{ severity::info };

    std::mutex _mutex;
};

template <typename... ts>
struct log_entry final
{
    log_entry(log& instance,
              int32_t frame,
              severity severity,
              std::string_view tag,
              std::string_view format,
              ts&&... format_args,
              const std::source_location& location = std::source_location::current())
    {
        instance.add_message(
            std::format("{:%d.%m.%Y_%T}", std::chrono::zoned_time{ std::chrono::current_zone(), std::chrono::system_clock::now() }),
            frame,
            severity,
            std::move(tag),
            std::vformat(format, std::make_format_args(format_args...)),
            location);
    }
};

template <typename... ts>
log_entry(log&, int32_t, severity, std::string_view, std::string_view, ts&&...) -> log_entry<ts...>;

}
