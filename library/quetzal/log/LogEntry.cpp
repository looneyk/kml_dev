//------------------------------------------------------------------------------
// log
// LogEntry.cpp
//------------------------------------------------------------------------------

#include "LogEntry.hpp"
#include <format>

//------------------------------------------------------------------------------
std::string quetzal::log::format_entry(const LogEntry& entry)
{
    auto time = std::chrono::time_point_cast<std::chrono::milliseconds>(entry.time);
    return std::format("{:%FT%TZ} {:<8} {}: {}", time, to_string(entry.level), entry.category, entry.message);
}
