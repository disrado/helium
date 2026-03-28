#include "game/systems/log_system.hpp"

#include <print>


namespace ne
{

logging::logging()
    : _log{ std::function<void(std::string&&)>{ [] (std::string&& message) { std::println("{}", message); } } }
{
}

auto logging::log(std::chrono::zoned_time<std::chrono::duration<std::chrono::system_clock::rep, std::chrono::system_clock::period>> time,
                  severity severity,
                  std::string_view tag,
                  std::string_view message,
                  const std::source_location& location) -> void
{
    _log.add_message(time, he::world::get_frame_number(), severity, tag, message, location);
}

auto logging::tick(double dt) -> void
{
    he::system_base::tick(dt);

    _log.tick(dt);
}

}
