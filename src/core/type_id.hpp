#pragma once

#include <cstdint>

namespace he
{

using type_id_t = uint32_t;

// type id cannot be a function-local static variable, because multiple template instantiations
// in different libraries would result in duplicates. Use type name instead. Each major compiler
// has it's own function signature format (std::source_location), parsers should be wirtten
// accordingly

// provide functionality to get type name. Should be parsed from function signature
template<typename T>
auto type_name() -> std::string
{
}
}
