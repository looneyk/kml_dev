//------------------------------------------------------------------------------
// main.cpp
//
//------------------------------------------------------------------------------

#include <iostream>
#include "quetzal/io/InputFileStream.hpp"
#include "quetzal/io/OutputFileStream.hpp"
#include "quetzal/Arguments.hpp"
#include "quetzal/Exception.hpp"
#include "quetzal/iio/iio.hpp"
#include "quetzal/file_util.hpp"

namespace
{

    const std::string ArgInfo = "Info";
    const std::string ArgTransform = "Transform";

    //----------------------------------------------------------------------
    void usage()
    {
        std::cout << std::endl;
        std::cout << "iio_test" << std::endl;
        std::cout << std::endl;
        std::cout << "iio_test <options> file <file>" << std::endl;
        std::cout << std::endl;
        std::cout << "<options>" << std::endl;
        std::cout << "    -i      Info" << std::endl;
        std::cout << "    -t      Transform" << std::endl;

        return;
    }

    //----------------------------------------------------------------------
    void test_transform(const quetzal::iio::Image& imageIn, quetzal::iio::Image& imageOut)
    {
        if (&imageIn != &imageOut)
        {
            imageOut = imageIn;
        }

        quetzal::iio::Image::raster_type::iterator iOut = imageOut.raster().begin();

        quetzal::iio::Image::raster_type::const_iterator iEnd = imageIn.raster().end();
        for (quetzal::iio::Image::raster_type::const_iterator iIn = imageIn.raster().begin(); iIn != iEnd; ++iIn)
        {
            quetzal::Color color(*iIn);
//            color.set_red(1.);
//            color.set_green(1.);
//            color.set_blue(1.);

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
        quetzal::iio::Image image;

        if (args.positional() < 1)
        {
            usage();
            return 1;
        }

        std::string fileIn = args.positional_value(0);
        quetzal::io::InputFileStream input(fileIn);
        if (input.fail())
        {
            std::cout << "Error opening input file: " << fileIn << std::endl;
        }

        if (args.named_value<bool>(ArgInfo))
        {
            quetzal::iio::read(input, image);

//            std::cout << image;
        }
        else if (args.named_value<bool>(ArgTransform))
        {
            if (args.positional() != 2)
            {
                usage();
                return 1;
            }

            quetzal::iio::read(input, image);

//            std::cout << image;

            test_transform(image, image);

//            std::cout << image;

            std::string fileOut = args.positional_value(1);

            quetzal::io::OutputFileStream output(fileOut);
            if (output.fail())
            {
                std::cout << "Error opening output file: " << fileOut << std::endl;
            }

            quetzal::iio::write(output, image);
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
