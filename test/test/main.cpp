
#include <string>
#include <sstream>
#include <vector>
#include <fstream>
#include <iostream>
#include <string_view>

using namespace std;

namespace
{

//    double Y = 0.2126 * r + 0.7152 * g + 0.0722 * b;
//    double Y = 0.299 * r + 0.587 * g + 0.114 * b

    const double Yr = 0.299;
    const double Yg = 0.587;
    const double Yb = 0.114;

    vector<vector<double>> vv =
    {
        {1.0, 0.0, 0.0},
        {1.0, .125, 0.0},
        {1.0, 1.0, 0.0},
        {0.0, 1.0, 0.0},
        {0.0, 1.0, 1.0},
        {0.0, 0.0, 1.0},
        {.125, 0.0, 1.0},
        {1.0, 0.0, 1.0}
    };

} // namespace

int main(int argc, char* argv[])
{
    argc;
    argv;

	string_view z("zzz");

    ofstream ofs("t");
    ostream& os = ofs; // cout

    double Ymin = 1.;

    for (const auto& v : vv)
    {
        double r = v[0];
        double g = v[1];
        double b = v[2];

        double Y = Yr * r + Yg * g + Yb * b;
        os << r << "\t" << g << "\t" << b << "\t" << Y << endl;
        if (Y < Ymin)
        {
            Ymin = Y;
        }
    }

    os << Ymin << endl;

    for (const auto& v : vv)
    {
        double r = v[0];
        double g = v[1];
        double b = v[2];
        double Y = Yr * r + Yg * g + Yb * b;
        os << r << "\t" << g << "\t" << b << "\t" << Y << "\t";

        r *= Ymin / Y;
        g *= Ymin / Y;
        b *= Ymin / Y;
        Y = Yr * r + Yg * g + Yb * b;

        os << r << "\t" << g << "\t" << b << "\t" << Y << "\t";
        os << hex << int(r * 256) << "\t" << int(g * 256) << "\t" << int(b * 256) << endl;
    }

/*
//    string s = "0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.5";
    string s = "0.5 0.5 0.5  0.5 0.5 0.5  0.5 0.5 0.5 0.5\n    0.5 0.5 0.5  0.5 0.5 0.5  0.5 0.5 0.5\n    0.5 0.5 0.5 0.5  0.5 0.5 0.5  0.5 0.5 0.5\n    0.5 0.5 0.5  0.5 0.5 0.5  0.5 0.5 0.5\n    0.5 0.5 0.5 ";

    vector<float> v(48);

    istringstream iss(s);
    for (int i = 0; i < 48; ++i)
    {
        iss >> v[i];
    }

    for (int i = 0; i < 48; ++i)
    {
        cout << i << " " << v[i] << endl;
    }
*/

//    string s;
//    getline(cin, s);

    return 0;
}
