#include "systems/logging.hpp"

#include <filesystem>
#include <print>
#include <magic_enum/magic_enum.hpp>


namespace ne
{

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

    auto log_message{
        std::format(
            "[{}][{}]{}[{}][{}:{}] {}",
            std::format("{:%d.%m.%Y_%T}", time),
            he::world::get_frame_number(),
            severity == severity::info ? std::string{} : std::format("[{}]", magic_enum::enum_name(severity)),
            tag,
            std::filesystem::path(location.file_name()).filename().string(),
            location.line(),
            message) };

    const auto guard{ std::lock_guard{ _mutex } };
    {
        _queue.push_back(std::move(log_message));
    }


    auto [_, _, _]{ std::tuple<int, int, int>{ 0, 0, 0} };
}

auto logging::set_severity_threshold(severity severity) -> void
{
    _threshold = severity;
}

auto logging::tick(double dt) -> void
{
    he::system_base::tick(dt);

    const auto process_entry_call{ [] (std::string&& message) { std::println("{}", message); } };

    const auto guard{ std::lock_guard{ _mutex } };
    {
        for (auto& message : _queue)
        {
            std::invoke(process_entry_call, std::move(message));
        }

        _queue.clear();
    }
}

}
