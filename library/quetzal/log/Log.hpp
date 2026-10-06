#if !defined(QUETZAL_LOG_LOG_HPP)
#define QUETZAL_LOG_LOG_HPP
//------------------------------------------------------------------------------
// log
// Log.hpp
//------------------------------------------------------------------------------

#include "LogEntry.hpp"
#include "LogLevel.hpp"
#include <format>
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace quetzal::log
{

    using log_sink_type = std::move_only_function<void(const LogEntry& entry)>;

    //--------------------------------------------------------------------------
    // Writes each entry to the sinks whose levels pass it. Thread-safe: an entry is written to every sink under one mutex,
    // so lines from two threads never interleave, and a sink needs no lock of its own
    class Log
    {
    public:

        Log();
        Log(const Log&) = delete;
        Log(Log&&) = delete;
        ~Log() = default;

        Log& operator=(const Log&) = delete;
        Log& operator=(Log&&) = delete;

        // A sink with no level writes every entry that its category's level passes. A sink is called with the log's mutex
        // locked, so it must not write to the same log
        void add_sink(log_sink_type sink, std::optional<LogLevel> level = std::nullopt);
        void clear_sinks();

        LogLevel level() const;
        void set_level(LogLevel level);

        // A category with no level has the log's level
        std::optional<LogLevel> category_level(std::string_view category) const;
        void set_category_level(std::string_view category, std::optional<LogLevel> level);

        // Returns true if at least one sink writes an entry of this level in this category
        bool writes(LogLevel level, std::string_view category) const;

        // Writes the entry to each sink for which its level is at least the higher severity of two levels: its category's
        // level, or the log's level if the category has none, and the sink's level. Requires a level other than Off and a category
        void write(LogLevel level, std::string_view category, std::string_view message);

        // Each formats the message only if at least one sink writes the entry
        template<typename... Args>
        void trace(std::string_view category, std::format_string<Args...> format, Args&&... args);
        template<typename... Args>
        void debug(std::string_view category, std::format_string<Args...> format, Args&&... args);
        template<typename... Args>
        void info(std::string_view category, std::format_string<Args...> format, Args&&... args);
        template<typename... Args>
        void warning(std::string_view category, std::format_string<Args...> format, Args&&... args);
        template<typename... Args>
        void error(std::string_view category, std::format_string<Args...> format, Args&&... args);
        template<typename... Args>
        void critical(std::string_view category, std::format_string<Args...> format, Args&&... args);

    private:

        struct InstalledSink
        {
            log_sink_type sink;
            std::optional<LogLevel> level;
        };

        template<typename... Args>
        void write_formatted(LogLevel level, std::string_view category, std::format_string<Args...> format, Args&&... args);

        // Each requires the mutex locked
        LogLevel category_level_or_default(std::string_view category) const;
        void write_entry(const LogEntry& entry);

        mutable std::mutex m_mutex;
        LogLevel m_level;
        std::map<std::string, LogLevel, std::less<>> m_category_levels; // <category, level>
        std::vector<InstalledSink> m_sinks;
        LogLevel m_lowest_sink_level; // Off with no sinks, and Trace with a sink that has no level
    };

    // Returns the process-wide log, constructed at Info on first use
    Log& process_log();

} // namespace quetzal::log

//------------------------------------------------------------------------------
template<typename... Args>
void quetzal::log::Log::trace(std::string_view category, std::format_string<Args...> format, Args&&... args)
{
    write_formatted(LogLevel::Trace, category, format, std::forward<Args>(args)...);
    return;
}

//------------------------------------------------------------------------------
template<typename... Args>
void quetzal::log::Log::debug(std::string_view category, std::format_string<Args...> format, Args&&... args)
{
    write_formatted(LogLevel::Debug, category, format, std::forward<Args>(args)...);
    return;
}

//------------------------------------------------------------------------------
template<typename... Args>
void quetzal::log::Log::info(std::string_view category, std::format_string<Args...> format, Args&&... args)
{
    write_formatted(LogLevel::Info, category, format, std::forward<Args>(args)...);
    return;
}

//------------------------------------------------------------------------------
template<typename... Args>
void quetzal::log::Log::warning(std::string_view category, std::format_string<Args...> format, Args&&... args)
{
    write_formatted(LogLevel::Warning, category, format, std::forward<Args>(args)...);
    return;
}

//------------------------------------------------------------------------------
template<typename... Args>
void quetzal::log::Log::error(std::string_view category, std::format_string<Args...> format, Args&&... args)
{
    write_formatted(LogLevel::Error, category, format, std::forward<Args>(args)...);
    return;
}

//------------------------------------------------------------------------------
template<typename... Args>
void quetzal::log::Log::critical(std::string_view category, std::format_string<Args...> format, Args&&... args)
{
    write_formatted(LogLevel::Critical, category, format, std::forward<Args>(args)...);
    return;
}

//------------------------------------------------------------------------------
template<typename... Args>
void quetzal::log::Log::write_formatted(LogLevel level, std::string_view category, std::format_string<Args...> format, Args&&... args)
{
    if (writes(level, category))
    {
        write(level, category, std::format(format, std::forward<Args>(args)...));
    }

    return;
}

#endif // QUETZAL_LOG_LOG_HPP
