//------------------------------------------------------------------------------
// log
// Log.cpp
//------------------------------------------------------------------------------

#include "Log.hpp"
#include <algorithm>
#include <cassert>

//------------------------------------------------------------------------------
quetzal::log::Log::Log() :
    m_mutex(),
    m_level(LogLevel::Info),
    m_category_levels(),
    m_sinks(),
    m_lowest_sink_level(LogLevel::Off)
{
}

//------------------------------------------------------------------------------
void quetzal::log::Log::add_sink(log_sink_type sink, std::optional<LogLevel> level)
{
    std::scoped_lock lock(m_mutex);
    m_sinks.push_back({std::move(sink), level});
    m_lowest_sink_level = std::min(m_lowest_sink_level, level.value_or(LogLevel::Trace));
    return;
}

//------------------------------------------------------------------------------
void quetzal::log::Log::clear_sinks()
{
    std::scoped_lock lock(m_mutex);
    m_sinks.clear();
    m_lowest_sink_level = LogLevel::Off;
    return;
}

//------------------------------------------------------------------------------
quetzal::log::LogLevel quetzal::log::Log::level() const
{
    std::scoped_lock lock(m_mutex);
    return m_level;
}

//------------------------------------------------------------------------------
void quetzal::log::Log::set_level(LogLevel level)
{
    std::scoped_lock lock(m_mutex);
    m_level = level;
    return;
}

//------------------------------------------------------------------------------
std::optional<quetzal::log::LogLevel> quetzal::log::Log::category_level(std::string_view category) const
{
    std::scoped_lock lock(m_mutex);

    auto i = m_category_levels.find(category);
    if (i == m_category_levels.end())
    {
        return std::nullopt;
    }

    return i->second;
}

//------------------------------------------------------------------------------
void quetzal::log::Log::set_category_level(std::string_view category, std::optional<LogLevel> level)
{
    std::scoped_lock lock(m_mutex);

    if (!level)
    {
        if (auto i = m_category_levels.find(category); i != m_category_levels.end())
        {
            m_category_levels.erase(i);
        }

        return;
    }

    m_category_levels.insert_or_assign(std::string(category), *level);
    return;
}

//------------------------------------------------------------------------------
bool quetzal::log::Log::writes(LogLevel level, std::string_view category) const
{
    std::scoped_lock lock(m_mutex);
    return level >= category_level_or_default(category) && level >= m_lowest_sink_level;
}

//------------------------------------------------------------------------------
void quetzal::log::Log::write(LogLevel level, std::string_view category, std::string_view message)
{
    assert(level != LogLevel::Off && !category.empty());

    if (!writes(level, category))
    {
        return;
    }

    LogEntry entry{std::chrono::system_clock::now(), level, std::string(category), std::string(message), std::this_thread::get_id()};

    std::scoped_lock lock(m_mutex);
    write_entry(entry);
    return;
}

//------------------------------------------------------------------------------
quetzal::log::LogLevel quetzal::log::Log::category_level_or_default(std::string_view category) const
{
    auto i = m_category_levels.find(category);
    return i != m_category_levels.end() ? i->second : m_level;
}

//------------------------------------------------------------------------------
void quetzal::log::Log::write_entry(const LogEntry& entry)
{
    LogLevel category_level = category_level_or_default(entry.category);

    for (auto& installed_sink : m_sinks)
    {
        LogLevel sink_threshold = installed_sink.level ? std::max(category_level, *installed_sink.level) : category_level;
        if (entry.level >= sink_threshold)
        {
            installed_sink.sink(entry);
        }
    }

    return;
}

//------------------------------------------------------------------------------
quetzal::log::Log& quetzal::log::logger()
{
    static Log s_logger;
    return s_logger;
}
