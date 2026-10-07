//------------------------------------------------------------------------------
// file_util.cpp
//
//------------------------------------------------------------------------------

#include "file_util.hpp"
#include <stdexcept>
#include <vector>
#include <cstdio>
#include <cassert>

#if defined(_WIN32)
#if !defined(WIN32_LEAN_AND_MEAN)
#define WIN32_LEAN_AND_MEAN
#endif
#if !defined(NOMINMAX)
#define NOMINMAX
#endif
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#endif

using namespace std;

//------------------------------------------------------------------------------
filesystem::path quetzal::executable_directory()
{
#if defined(_WIN32)
    // GetModuleFileNameW truncates a path that does not fit, so the buffer grows until it does
    vector<wchar_t> buffer(MAX_PATH);
    for (;;)
    {
        DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (length == 0)
        {
            throw runtime_error("the executable's path cannot be read");
        }

        if (length < buffer.size())
        {
            return filesystem::path(wstring(buffer.data(), length)).parent_path();
        }

        buffer.resize(buffer.size() * 2);
    }
#else
    throw runtime_error("the executable's directory is not implemented on this platform");
#endif
}

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
