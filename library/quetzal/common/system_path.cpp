//------------------------------------------------------------------------------
// system_path.cpp
//------------------------------------------------------------------------------

#include "system_path.hpp"

using namespace std;

//------------------------------------------------------------------------------
filesystem::path quetzal::system_path(REFKNOWNFOLDERID rfid)
{
    filesystem::path p;

    wchar_t* pwc = nullptr;
    HRESULT hr = SHGetKnownFolderPath(rfid, 0, nullptr, &pwc);
    if (pwc != nullptr)
    {
        if (SUCCEEDED(hr))
        {
            p.assign(pwc);
        }

        CoTaskMemFree(pwc);
    }

    return p;
}
