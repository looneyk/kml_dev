//------------------------------------------------------------------------------
// main.cpp
//
//------------------------------------------------------------------------------

#include <iostream>
#include "quetzal/io/InputFileStream.hpp"
#include "quetzal/io/OutputFileStream.hpp"
#include "quetzal/Arguments.hpp"
#include "quetzal/Exception.hpp"
#include "quetzal/gio/gio.hpp"
#include "quetzal/file_util.hpp"

namespace
{

    const std::string ArgInfo = "Info";
    const std::string ArgTransform = "Transform";

    //----------------------------------------------------------------------
    void usage()
    {
        std::cout << std::endl;
        std::cout << "gio_test" << std::endl;
        std::cout << std::endl;
        std::cout << "gio_test <options> file <file>" << std::endl;
        std::cout << std::endl;
        std::cout << "<options>" << std::endl;
        std::cout << "    -i      Info" << std::endl;
        std::cout << "    -t      Transform" << std::endl;

        return;
    }

    //----------------------------------------------------------------------
    void test_transform(const quetzal::gio::BitMap& bmIn, quetzal::gio::BitMap& bmOut)
    {
        if (&bmIn != &bmOut)
        {
            bmOut = bmIn;
        }

        quetzal::gio::BitMap::array_type::iterator iOut = bmOut.data().begin();

        quetzal::gio::BitMap::array_type::const_iterator iEnd = bmIn.data().end();
        for (quetzal::gio::BitMap::array_type::const_iterator iIn = bmIn.data().begin(); iIn != iEnd; ++iIn)
        {
            quetzal::Color color(*iIn);
            color.set_red(1.);
        //    color.set_green(1.);
        //    color.set_blue(1.);

            *iOut++ = color.packed();
        }
    }

} // namespace

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    // Parse arguments

    quetzal::Arguments args;
    args.register_argument(ArgInfo, "", "-i", 0, false, "false", "true");
    args.register_argument(ArgTransform, "", "-t", 0, false, "false", "true");

    args.parse(argc, argv);

//    std::cout << args << std::endl;

    try
    {
        quetzal::gio::BitMap bm;

        if (args.positional() < 1)
        {
            usage();
            return 1;
        }

        std::string fileIn = args.positional_value(0);
        quetzal::io::InputFileStream input(fileIn);
        if (!input)
        {
            std::cout << "Error opening input file: " << fileIn << std::endl;
        }

        std::string formatId = quetzal::file_extension(fileIn); // May need more complex processing? ...

        if (args.named_value<bool>(ArgInfo))
        {
            quetzal::gio::read(input, bm, formatId);

//            std::cout << bm;
        }
        else if (args.named_value<bool>(ArgTransform))
        {
            if (args.positional() != 2)
            {
                usage();
                return 1;
            }

            quetzal::gio::read(input, bm, formatId);

//            std::cout << bm;

            test_transform(bm, bm);

//            std::cout << bm;

            std::string fileOut = args.positional_value(1);

            quetzal::io::OutputFileStream output(fileOut);
            if (!output)
            {
                std::cout << "Error opening output file: " << fileOut << std::endl;
            }

            quetzal::gio::write(output, bm, formatId);
        }
        else
        {
            std::cout << "No operation specified" << std::endl;
        }
    }
    catch (quetzal::Exception& e)
    {
        std::cout << e.what() << " at " << e.file() << " line " << e.line() << std::endl;
    }

    return 0;
}
