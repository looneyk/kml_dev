//------------------------------------------------------------------------------
// log
// LogLevel.cpp
//------------------------------------------------------------------------------

#include "LogLevel.hpp"
#include <array>
#include <cassert>
#include <utility>

namespace
{

    using namespace quetzal::log;

    constexpr std::array<std::pair<LogLevel, std::string_view>, 7> level_names
    {{
        {LogLevel::Trace, "trace"},
        {LogLevel::Debug, "debug"},
        {LogLevel::Info, "info"},
        {LogLevel::Warning, "warning"},
        {LogLevel::Error, "error"},
        {LogLevel::Critical, "critical"},
        {LogLevel::Off, "off"}
    }};

} // namespace

//------------------------------------------------------------------------------
std::string_view quetzal::log::to_string(LogLevel level)
{
    for (const auto& [level_named, name] : level_names)
    {
        if (level_named == level)
        {
            return name;
        }
    }

    assert(false);
    return {};
}

//------------------------------------------------------------------------------
std::optional<quetzal::log::LogLevel> quetzal::log::to_log_level(std::string_view name)
{
    for (const auto& [level, level_name] : level_names)
    {
        if (level_name == name)
        {
            return level;
        }
    }

    return std::nullopt;
}
