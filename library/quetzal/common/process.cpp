//------------------------------------------------------------------------------
// common
// process.cpp
//------------------------------------------------------------------------------

#include "process.hpp"
#include <process.h>
#include <windows.h>
#include <shellapi.h>

using namespace std;

//------------------------------------------------------------------------------
int quetzal::run_process(const filesystem::path& app_path, const vector<string>& args)
{
    vector<string> arguments;
    arguments.push_back(app_path.string());
    arguments.insert(arguments.end(), args.begin(), args.end());

    vector<const char*> argv;
    for (const auto& argument : arguments)
    {
        argv.push_back(argument.c_str());
    }
    argv.push_back(nullptr);

    return static_cast<int>(_spawnv(_P_WAIT, app_path.string().c_str(), argv.data()));
}

//------------------------------------------------------------------------------
void quetzal::open_with_default_application(const filesystem::path& path)
{
    ShellExecuteW(nullptr, L"open", path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    return;
}
