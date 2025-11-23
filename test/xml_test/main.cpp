//------------------------------------------------------------------------------
// xml_test
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/xml/xml.hpp"
#include "quetzal/Exception.hpp"
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

using namespace std;
using namespace quetzal;

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        cout << "Missing filename" << endl;
        return 0;
    }

    try
    {
        string filename = argv[1]; 

        xml::Document document;

        ifstream ifs(filename, ios_base::binary);
        if (!ifs.is_open())
        {
            ostringstream oss;
            oss << "Error opening XML file: " << filename << ": ";
            throw Exception(oss.str(), __FILE__, __LINE__);
        }

        ifs >> document;

        cout << document;
    }
    catch (exception& e)
    {
        cout << e.what() << endl;
    }

    return 0;
}
