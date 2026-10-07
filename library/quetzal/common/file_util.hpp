#if !defined(QUETZAL_FILE_UTIL_HPP)
#define QUETZAL_FILE_UTIL_HPP
//------------------------------------------------------------------------------
// file_util.hpp
// include system_path here? only if it can be without reference to Windows? ...
//
//------------------------------------------------------------------------------

#include <filesystem>
#include <iostream>
#include <istream>
#include <string>

namespace quetzal
{

    // The path of the running executable
    // Throws std::runtime_error on error
    // Implemented for Windows only
    std::filesystem::path executable_directory();

    //--------------------------------------------------------------------------
    // Reads a line, dropping a trailing cr, so a file with crlf endings reads the same on every platform
    inline std::istream& read_line(std::istream& is, std::string& line)
    {
        std::getline(is, line);

        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }

        return is;
    }

    // Redirect stdout or stderr to another stream

    //--------------------------------------------------------------------------
    class OstreamRedirect
    {
    public:

        OstreamRedirect(std::ostream& os_orig, std::ostream& os_redirect);
        OstreamRedirect(const OstreamRedirect&) = delete;
        OstreamRedirect(OstreamRedirect&&) = delete;
        OstreamRedirect& operator=(const OstreamRedirect&) = delete;
        OstreamRedirect& operator=(OstreamRedirect&&) = delete;
        ~OstreamRedirect();

    private:

        std::ostream& m_os_orig;
        std::streambuf* m_buffer_prev;
    };


    // Change stdout and stderr to binary mode to produce Unix line endings
    void set_output_file_lf_endings();

//    int set_lf_endings(std::ostream& os); // returns mode value as defined in fcntl.h
//    void set_mode(std::ostream& os, int mode); // mode value as defined in fcntl.h or return value from above
// or do like OstreamRedirect, or add a mode change option to OstreamRedirect? ...

} // namespace quetzal

#endif  //  QUETZAL_FILE_UTIL_HPP
