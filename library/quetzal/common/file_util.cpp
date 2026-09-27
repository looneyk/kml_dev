//------------------------------------------------------------------------------
// file_util.cpp
//
//------------------------------------------------------------------------------

#include "file_util.hpp"
#include <cstdio>
#include <cassert>

#if defined(_WIN32)
#include <fcntl.h>
#include <io.h>
#endif

using namespace std;

//------------------------------------------------------------------------------
quetzal::OstreamRedirect::OstreamRedirect(std::ostream& os_orig, std::ostream& os_redirect) :
    m_os_orig(os_orig),
    m_buffer_prev(m_os_orig.rdbuf(os_redirect.rdbuf()))
{ }

//------------------------------------------------------------------------------
quetzal::OstreamRedirect::~OstreamRedirect()
{
    m_os_orig.rdbuf(m_buffer_prev);
}    

//------------------------------------------------------------------------------
void quetzal::set_output_file_lf_endings()
{
#if defined(_WIN32)
    cout.flush();
    cerr.flush();

    // The calls have to be made outside the assert so that they still happen when NDEBUG is defined
    [[maybe_unused]] int mode_stdout = _setmode(_fileno(stdout), _O_BINARY);
    assert(mode_stdout != -1);

    [[maybe_unused]] int mode_stderr = _setmode(_fileno(stderr), _O_BINARY);
    assert(mode_stderr != -1);
#endif

    // Platforms other than Windows do not translate line endings on output
    return;
}
/*
//------------------------------------------------------------------------------
int quetzal::set_lf_endings(std::ostream& os)
{
    os.flush();
    auto handle = os.native_handle();
    int fd = _open_osfhandle(reinterpret_cast<intptr_t>(handle), 0);
    int mode_prev = _setmode(fd, _O_BINARY);
    assert(mode_prev != -1);
    return mode_prev;
}
*/
