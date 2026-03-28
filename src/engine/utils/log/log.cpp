#include "log.hpp"

#include <chrono>
#include <filesystem>
#include <magic_enum/magic_enum.hpp>


namespace he
{
log::log(std::function<void(std::string&&)> output_call)
    : _message_queue{}
    , _output_call{ std::move(output_call) }
{
}

auto log::add_message(std::chrono::zoned_time<std::chrono::duration<std::chrono::system_clock::rep, std::chrono::system_clock::period>> time,
                      int32_t frame,
                      severity severity,
                      std::string_view tag,
                      std::string_view message,
                      const std::source_location& location) -> void
{
    if (severity < _threshold)
    {
        return;
    }

    const auto guard{ std::lock_guard{ _mutex } };
    {
        _message_queue.emplace(
            std::format(
                "[{}][{}]{}[{}][{}:{}] {}",
                std::format("{:%d.%m.%Y_%T}", time),
                frame,
                severity == severity::info ? std::string{} : std::format("[{}]", magic_enum::enum_name(severity)),
                tag,
                std::filesystem::path(location.file_name()).filename().string(),
                location.line(),
                message));
    };
}

auto log::set_severity_threshold(severity severity) -> void
{
    _threshold = severity;
}

auto log::tick(double dt) -> void
{
    const auto guard{ std::lock_guard{ _mutex } };
    {
        for (; !_message_queue.empty(); _message_queue.pop())
        {
            std::invoke(_output_call, std::move(_message_queue.front()));
        }
    }
}

}
