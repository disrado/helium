#include <print>
#include <ranges>

#include "helium/systems/root_system.hpp"
#include "helium/core/world.hpp"
#include "helium/utils/string_literal.hpp"
#include "helium/systems/logging.hpp"
#include "helium/utils/type_traits/type_name.hpp"
#include "utils/containers/ordered_tree.hpp"


struct container
{
    template <typename t>
    auto bar(const std::string_view) noexcept(false) -> std::pair<int, int>
    {
        he::world::instance().get<he::root_system>().add_child<ne::logging>();

        [[maybe_unused]] constexpr auto tag{ he::string_literal{ "logging_system" } };

        ne::log{ info, "main", "format: {}, {}, {}", "message", 10, std::string{} };

        ne::log{ warning, "main", "formated message" };

        ne::log{ error, "main", "unexpected error" };

        ne::log{ error, "main", "format: {}:{}", std::chrono::system_clock::now(), 10 };

        ne::log{ info, he::name_of(this), "format: {}:{}:{}", "unexpected error", 10, "" };

        he::world::instance().tick();

        return std::pair<int, int>{ 0, 0 };
    }
};

auto main() -> int
{
    return 0;
}
