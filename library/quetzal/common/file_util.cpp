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
void quetzal::set_output_file_endings()
{
    //#pragma warning(suppress : 4996)
    //assert(freopen(nullptr, "wb", stdout) != nullptr);
    assert(_setmode(_fileno(stdout), _O_BINARY) != -1);
    assert(_setmode(_fileno(stderr), _O_BINARY) != -1);
}
