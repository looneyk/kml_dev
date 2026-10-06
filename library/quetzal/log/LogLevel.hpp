#if !defined(QUETZAL_LOG_LOGLEVEL_HPP)
#define QUETZAL_LOG_LOGLEVEL_HPP
//------------------------------------------------------------------------------
// log
// LogLevel.hpp
//------------------------------------------------------------------------------

#include <optional>
#include <string_view>

namespace quetzal::log
{

    //--------------------------------------------------------------------------
    // From least to most severe. Off is only a threshold: a log, category or sink at Off writes nothing
    enum class LogLevel
    {
        Trace,
        Debug,
        Info,
        Warning,
        Error,
        Critical,
        Off
    };

    // The level's name in lowercase: "trace", "warning", "off"
    std::string_view to_string(LogLevel level);

    // Returns the level whose name to_string returns, or std::nullopt if there is none
    std::optional<LogLevel> to_log_level(std::string_view name);

} // namespace quetzal::log

#endif // QUETZAL_LOG_LOGLEVEL_HPP
