#if !defined(QUETZAL_FILE_UTIL_HPP)
#define QUETZAL_FILE_UTIL_HPP
//------------------------------------------------------------------------------
// file_util.hpp
// include system_path here? only if it can be without reference to Windows? ...
//
//------------------------------------------------------------------------------

#include <iostream>

namespace quetzal
{

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
    void set_output_file_endings();

} // namespace quetzal

#endif  //  QUETZAL_FILE_UTIL_HPP
