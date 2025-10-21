//------------------------------------------------------------------------------
// dungeon_test
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/common/FileValidator.hpp"
#include "quetzal/dungeon/Level.hpp"
#include "quetzal/dungeon/load_level.hpp"
#include <fstream>

using namespace std;
using namespace quetzal;
using namespace quetzal::dungeon;

namespace
{

    static const filesystem::path dirIn = "in/";
    static const filesystem::path dirOut = "out/";
    static const filesystem::path dirReference = "../../../dev_/application/dungeon_test/out/";

} // namespace

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    argc;
    argv;

    FileValidator validator(dirOut, dirReference);

    Level level(0, "Level 0");
    load_level(dirIn / "level0.txt", level);

    ofstream os(dirOut / "Level_0.level", ios_base::binary);
    os << level;

    validator.report_results();
    return 0;
}
