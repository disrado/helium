#include "gd_logging.h"

#include "core/logging.hpp"

#include <godot_cpp/variant/utility_functions.hpp>

#include <filesystem>
#include <format>
#include <magic_enum/magic_enum.hpp>


namespace
{

auto format_log_entry(severity level, const he::log_entry& entry) -> std::string
{
    return std::format(
        "[{}][{}][{}][{}][{}:{}] {}",
        std::format("{:%d.%m.%Y_%T}", entry.time),
        0,
        magic_enum::enum_name(level),
        entry.tag,
        std::filesystem::path(entry.location.file_name()).filename().string(),
        entry.location.line(),
        entry.message);
}

}

namespace he
{

auto register_gd_logging_dispatchers() -> void
{
    logging::instance().set_severity_dispatcher(
        error,
        [] (const log_entry& entry)
        {
            godot::UtilityFunctions::push_error(format_log_entry(error, entry).c_str());
        });

    logging::instance().set_severity_dispatcher(
        warning,
        [] (const log_entry& entry)
        {
            godot::UtilityFunctions::push_warning(format_log_entry(warning, entry).c_str());
        });

    logging::instance().set_severity_dispatcher(
        info,
        [] (const log_entry& entry)
        {
            godot::UtilityFunctions::print(format_log_entry(info, entry).c_str());
        });
}

}
