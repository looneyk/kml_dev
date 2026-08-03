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

    // Exit status is the ResultStatus value itself, so a caller can tell the passing cases apart and
    // report the worst of a run. Beyond them means no comparison was made at all
    constexpr int status_no_result = int(FileValidator::ResultStatus::Different) + 1;

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
        cout << endl;
        cout << "Exit status is the FileValidator result value, ordered least to most severe:" << endl;
        cout << "  0  all files equal" << endl;
        cout << "  1  files equal, some unchanged" << endl;
        cout << "  2  files equal, some missing" << endl;
        cout << "  3  differences found" << endl;
        cout << "  4  no comparison made, the executable could not be run" << endl;

        return;
    }

} // namespace

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    if (argc < 4)
    {
        usage();
        return status_no_result;
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
        return status_no_result;
    }

    validator.report_results();

    return int(validator.result());
}
