#if !defined(QUETZAL_LOG_LOGENTRY_HPP)
#define QUETZAL_LOG_LOGENTRY_HPP
//------------------------------------------------------------------------------
// log
// LogEntry.hpp
//------------------------------------------------------------------------------

#include "LogLevel.hpp"
#include <chrono>
#include <string>
#include <thread>

namespace quetzal::log
{

    //--------------------------------------------------------------------------
    struct LogEntry
    {
        std::chrono::system_clock::time_point time;
        LogLevel level = LogLevel::Info;
        std::string category; // The library or part of the program that wrote the entry
        std::string message;
        std::thread::id thread_id;
    };

    // Returns the entry as one line of text, with no line ending: its time in ISO 8601 UTC to the millisecond, its level,
    // its category, and its message
    std::string format_entry(const LogEntry& entry);

} // namespace quetzal::log

#endif // QUETZAL_LOG_LOGENTRY_HPP
