//------------------------------------------------------------------------------
// file_util.cpp
//
//------------------------------------------------------------------------------

#include "file_util.hpp"
#include <cstdio>
#include <cassert>
#include <fcntl.h>
#include <io.h>

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
    cout.flush();
    cerr.flush();
    assert(_setmode(_fileno(stdout), _O_BINARY) != -1);
    assert(_setmode(_fileno(stderr), _O_BINARY) != -1);
    // set_lf_endings(cout);
    // set_lf_endings(cerr);
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
