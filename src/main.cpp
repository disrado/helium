#include <print>

#include "core/type_name.hpp"


auto main() -> int
{
    std::println("type: {}", he::type_name<std::size_t>());
}
