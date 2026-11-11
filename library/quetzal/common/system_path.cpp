//------------------------------------------------------------------------------
// system_path.cpp
//------------------------------------------------------------------------------

#include "system_path.hpp"
#include <cstdlib>
#include <stdexcept>
#include <string>

using namespace std;

#if defined(_WIN32)

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
#endif

//------------------------------------------------------------------------------
optional<filesystem::path> quetzal::environment_path(const char* name)
{
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4996) // MSVC deprecates the standard getenv in favor of its own _dupenv_s
#endif
    const char* value = getenv(name);
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
    if (value == nullptr || *value == '\0')
    {
        return nullopt;
    }

    return filesystem::path(value);
}

//------------------------------------------------------------------------------
filesystem::path quetzal::required_environment_path(const char* name)
{
    optional<filesystem::path> path = environment_path(name);
    if (!path.has_value())
    {
        throw runtime_error(string("the environment variable ") + name + " is not set");
    }

    return path.value();
}

//------------------------------------------------------------------------------
filesystem::path quetzal::application_data_directory()
{
#if defined(_WIN32)
    return required_environment_path("LOCALAPPDATA");
#elif defined(__APPLE__)
    return required_environment_path("HOME") / "Library/Application Support";
#else
    optional<filesystem::path> data_home = environment_path("XDG_DATA_HOME");
    return data_home.has_value() ? data_home.value() : required_environment_path("HOME") / ".local/share";
#endif
}
