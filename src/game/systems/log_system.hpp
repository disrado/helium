#pragma once

#include "engine/core/system_base.hpp"
#include "engine/utils/log/log.hpp"


namespace ne
{

class logging: public he::system_base
{
public:
    logging();

public:
    auto log(std::chrono::zoned_time<std::chrono::duration<std::chrono::system_clock::rep, std::chrono::system_clock::period>> time,
             severity severity,
             std::string_view tag,
             std::string_view message,
             const std::source_location& location) -> void;

protected:
    auto tick(double dt) -> void override;

private:
    he::log _log;
};

template <severity severity>
struct log_base
{
    template <typename... ts>
    log_base(std::string_view tag,
             std::string_view format,
             const std::source_location& location,
             ts&&... args)
    {
        he::world::get_system<logging>().log(
            std::chrono::zoned_time{ std::chrono::current_zone(), std::chrono::system_clock::now() },
            severity,
            tag,
            std::vformat(format, std::make_format_args(args...)),
            location);
    }
};

template <typename... ts>
struct info final: log_base<severity::info>
{
    info(std::string_view tag, std::string_view format, ts&&... args, const std::source_location& location = std::source_location::current())
        : log_base{ tag, format, location, std::forward<ts>(args)... }
    {
    }
};

template <typename... ts>
info(std::string_view, std::string_view, ts&&...) -> info<ts...>;


template <typename... ts>
struct warning final: log_base<severity::warning>
{
    warning(std::string_view tag, std::string_view format, ts&&... args, const std::source_location& location = std::source_location::current())
        : log_base{ tag, format, location, std::forward<ts>(args)... }
    {
    }
};

template <typename... ts>
warning(std::string_view, std::string_view, ts&&...) -> warning<ts...>;


template <typename... ts>
struct error final: log_base<severity::error>
{
    error(std::string_view tag, std::string_view format, ts&&... args, const std::source_location& location = std::source_location::current())
        : log_base{ tag, format, location, std::forward<ts>(args)... }
    {
    }
};

template <typename... ts>
error(std::string_view, std::string_view, ts&&...) -> error<ts...>;

}
