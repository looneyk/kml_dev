#if !defined(QUETZAL_LOG_STREAMLOGSINK_HPP)
#define QUETZAL_LOG_STREAMLOGSINK_HPP
//------------------------------------------------------------------------------
// log
// StreamLogSink.hpp
//------------------------------------------------------------------------------

#include "LogEntry.hpp"
#include <ostream>

namespace quetzal::log
{

    //--------------------------------------------------------------------------
    // Writes each entry to a stream, such as std::clog, as a line of format_entry, ending in '\n' and flushed. The stream must outlive the sink
    class StreamLogSink
    {
    public:

        explicit StreamLogSink(std::ostream& stream);
        StreamLogSink(const StreamLogSink&) = default;
        StreamLogSink(StreamLogSink&&) = default;
        ~StreamLogSink() = default;

        StreamLogSink& operator=(const StreamLogSink&) = delete;
        StreamLogSink& operator=(StreamLogSink&&) = delete;

        void operator()(const LogEntry& entry);

    private:

        std::ostream& m_stream;
    };

} // namespace quetzal::log

#endif // QUETZAL_LOG_STREAMLOGSINK_HPP
