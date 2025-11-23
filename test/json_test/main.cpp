//------------------------------------------------------------------------------
// main.cpp
//------------------------------------------------------------------------------

#define __attribute__(x)
#include "tao/json.hh"
#include <fstream>
#include <iostream>
//#include <iomanip>

using namespace std;

namespace
{

    const string s_dot_correction_filename = "dot_correction.json";

} // namespace

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    argc;
    argv;

    try
    {
//        ifstream ifs(s_dot_correction_filename);
//        const auto& value = tao::json::from_stream(ifs);
        /*const*/ auto value = tao::json::parse_file(s_dot_correction_filename);
//        validate_dots_format(value, 1); // dot correction, color values; same file format, something more general than dots?

        if (!value.is_object())
        {
            cout << "not object" << endl;
            return 1;
        }

//        const tao::json::basic_value::object_t& o = value.get_object();
        /*const*/ auto& o = value.get_object();

//xxx;        
        int w = o["width"].as<int>();
        int h = o["height"].as<int>();
        int n = o["components"].as<int>();
        /*const*/ auto& rows_value = o["data"];
        if (!rows_value.is_array())
        {
            cout << "not y array" << endl;
            return 1;
        }

//        const tao::json::basic_value::array_t& rows = rows_value.get_array();
        const auto rows = rows_value.get_array();

        if (n != 1)
        {
            cout << "n" << endl;
            return 1;
        }

        if (rows.size() != h)
        {
            cout << "h" << endl;
            return 1;
        }

//yyy;
//        for (const auto& row_value : rows)
//        for (auto i = rows.begin(); i != rows.end(); ++i)
        for (size_t i = 0; i != rows.size(); ++i)
        {
//zzz;
            auto row_value = rows[i];

            if (!row_value.is_array())
            {
                cout << "not x array" << endl;
                return 1;
            }

//            const tao::json::basic_value::array_t& row = row_value.get_array();
            const auto& row = row_value.get_array();

            if (row.size() != w)
            {
                cout << "w" << endl;
                return 1;
            }

//ppp;
//            for (const auto& dot : row)
            for (size_t j = 0; j != rows.size(); ++j)
            {
                auto dot = rows[j];

                cout << dot.as<double>() << " ";
            }
//qqq;

            cout << endl;
        }
    }
    catch (exception& e)
    {
        cout << e.what() << endl;
        return 1;
    }

    return 0;
}
