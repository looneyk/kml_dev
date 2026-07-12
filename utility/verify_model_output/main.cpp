//------------------------------------------------------------------------------
// verify_model_output
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/common/FileValidator.hpp"
#include "quetzal/common/process.hpp"
#include <filesystem>
#include <iostream>
#include <string>

using namespace std;
using namespace quetzal;

namespace
{

    //--------------------------------------------------------------------------
    void usage()
    {
        cout << endl;
        cout << "verify_model_output" << endl;
        cout << endl;
        cout << "Runs a model-generation executable and checks its output against a" << endl;
        cout << "reference directory. A snapshot of the output directory is created before the" << endl;
        cout << "executable runs, so newly written files can be distinguished from stale" << endl;
        cout << "files left over from a previous run." << endl;
        cout << endl;
        cout << "verify_model_output <app_exe> <out_dir> <reference_dir> [app_name]" << endl;

        return;
    }

} // namespace

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    if (argc < 4)
    {
        usage();
        return 1;
    }

    string app_exe = argv[1];
    filesystem::path out_dir = argv[2];
    filesystem::path reference_dir = argv[3];
    string app_name = argc > 4 ? argv[4] : "";

    // FileValidator must be constructed (snapshotting out_dir) before the app runs.
    FileValidator validator(out_dir, reference_dir, app_name);

    int status = run_process(app_exe);
    if (status != 0)
    {
        cerr << "Could not run " << app_exe << " (status " << status << ")" << endl;
        return 1;
    }

    int n = validator.report_results();

    return n >= 0 ? 0 : 1;
}
