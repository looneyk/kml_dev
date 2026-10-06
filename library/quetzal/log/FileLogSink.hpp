#if !defined(QUETZAL_LOG_FILELOGSINK_HPP)
#define QUETZAL_LOG_FILELOGSINK_HPP
//------------------------------------------------------------------------------
// log
// FileLogSink.hpp
//------------------------------------------------------------------------------

#include "LogEntry.hpp"
#include <filesystem>
#include <fstream>

namespace quetzal::log
{

    //--------------------------------------------------------------------------
    // Appends each entry to a file as a line of format_entry, ending in '\n' and flushed
    class FileLogSink
    {
    public:

        // Creates the file's directory if it does not exist. Throws std::runtime_error if the file cannot be opened
        explicit FileLogSink(const std::filesystem::path& filepath);
        FileLogSink(const FileLogSink&) = delete;
        FileLogSink(FileLogSink&&) = default;
        ~FileLogSink() = default;

        FileLogSink& operator=(const FileLogSink&) = delete;
        FileLogSink& operator=(FileLogSink&&) = default;

        void operator()(const LogEntry& entry);

    private:

        std::ofstream m_stream;
    };

} // namespace quetzal::log

#endif // QUETZAL_LOG_FILELOGSINK_HPP
