//------------------------------------------------------------------------------
// log
// StreamLogSink.cpp
//------------------------------------------------------------------------------

#include "StreamLogSink.hpp"

//------------------------------------------------------------------------------
quetzal::log::StreamLogSink::StreamLogSink(std::ostream& stream) :
    m_stream(stream)
{
}

//------------------------------------------------------------------------------
void quetzal::log::StreamLogSink::operator()(const LogEntry& entry)
{
    m_stream << format_entry(entry) << '\n';
    m_stream.flush();
    return;
}
