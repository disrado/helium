#include <print>

#include "core/type_info.hpp"


auto main() -> int
{
    std::println("type: {}", he::type_id<int>().index());
    std::println("type: {}", he::type_id<int>().name());

    std::println("type: {}", he::type_id<std::size_t>().index());
    std::println("type: {}", he::type_id<std::size_t>().name());

    std::println("type: {}", he::type_id<int&>().index());
    std::println("type: {}", he::type_id<int&>().name());

    std::println("type: {}", he::type_id<decltype([]{})>().index());
    std::println("type: {}", he::type_id<decltype([]{})>().name());

    std::println("type: {}", he::type_id<int>().index());
    std::println("type: {}", he::type_id<int>().name());
}
