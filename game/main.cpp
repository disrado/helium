#include "SDL3/SDL.h"

#include <print>
#include <stacktrace>

#include "helium/defs.hpp"
#include "helium/systems/root_system.hpp"
#include "helium/core/world.hpp"
#include "helium/utils/string_literal.hpp"
#include "helium/systems/logging.hpp"
#include "helium/utils/type_traits/type_name.hpp"


struct container
{
    template <typename t>
    static auto bar(const std::string_view _) noexcept(false) -> std::pair<int, int>
    {
        he::world::get_system<he::root_system>().add_subsystem<ne::logging>();

        const auto& world{ he::world::instance() };

        [[maybe_unused]] constexpr auto tag{ he::string_literal{ "logging_system" } };

        ne::info{ "tag", "format: {}", "message", 10, std::string{} };

        // ne::info<he::string_literal{ "tag" }, std::string, std::string>{ info, "format: {}",  std::string{ "message" }, std::string{ "message" } };
        //
        // ne::info{ "", "format: {}", "message", 10, std::string{} };

        ne::info{ "tag", "format: {}", "message", 10, std::string{} };
        ne::error{ "tag", "format: {}", "message", 10, std::string{} };

        ne::error{ ne::engine, "message" };

        ne::error{ ne::engine, std::format("formatted message is: {} + {}", 10, 4) };

        ne::info{ he::name_of(world), "" };

        he::world::instance().tick();

        return std::pair<int, int>{ 0, 0 };
    }
};

auto main() -> int
{
    if (auto ticks{ SDL_Time{} }; SDL_GetCurrentTime(&ticks))
    {
        std::println("current time: {}", ticks);
    }
    else
    {
        std::println("failed to retrieve current time");
    }

    container::bar<std::pair<int, int>>("");

    const auto& world{ he::world::instance() };

    std::println("size: {}", world._systems_cache.size());

    std::println("{}", std::stacktrace::current());

    return 0;
}
