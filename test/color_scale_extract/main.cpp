
#include <fstream>

using namespace std;

int main(int argc, char* argv[]
{
    ifstream ifs("c:\\kml\\download\\nuclide\\color_scale.ppm", ios_base::binary);
    ofstream ofs("c:\\kml\\download\\nuclide\\color_scale");

    ifs.seekg(14, ios_base::cur); // beginning of data
    ifs.seekg(30, ios_base::cur); // 10th column

    for (int i = 0; i < 557; ++i)
    {
        int r = ifs.get();
        int g = ifs.get();
        int b = ifs.get();

        ofs << r << " " << g << " " << b << endl;

        ifs.seekg(76 * 3, ios_base::cur);
    }

    return 0;
}
