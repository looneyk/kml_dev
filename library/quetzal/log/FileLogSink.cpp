//------------------------------------------------------------------------------
// log
// FileLogSink.cpp
//------------------------------------------------------------------------------

#include "FileLogSink.hpp"
#include <format>
#include <stdexcept>

//------------------------------------------------------------------------------
quetzal::log::FileLogSink::FileLogSink(const std::filesystem::path& filepath) :
    m_stream()
{
    if (filepath.has_parent_path())
    {
        std::filesystem::create_directories(filepath.parent_path());
    }

    m_stream.open(filepath, std::ios_base::binary | std::ios_base::app);
    if (!m_stream)
    {
        throw std::runtime_error(std::format("unable to open log file {}", filepath.string()));
    }
}

//------------------------------------------------------------------------------
void quetzal::log::FileLogSink::operator()(const LogEntry& entry)
{
    m_stream << format_entry(entry) << '\n';
    m_stream.flush();
    return;
}
