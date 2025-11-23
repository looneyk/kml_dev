//------------------------------------------------------------------------------
// main.cpp
//
//------------------------------------------------------------------------------

#include <iostream>
#include <algorithm>
#include "InputFileStream.hpp"
#include "OutputFileStream.hpp"
#include "Arguments.hpp"
#include "Exception.hpp"
#include "gio.hpp"
#include "file_util.hpp"
#include "matrix.hpp"

namespace
{

    typedef quetzal::basic_matrix<quetzal::Color::yuv_component_type> Matrix;

    const std::string ArgMedianWindow = "MedianWindow";
    const std::string ArgThreshold = "Threshold";

    //----------------------------------------------------------------------
    void usage()
    {
        std::cout << std::endl;
        std::cout << "dust_test" << std::endl;
        std::cout << std::endl;
        std::cout << "dust_test <options> fileIn fileOut" << std::endl;
        std::cout << std::endl;
        std::cout << "<options>" << std::endl;
        std::cout << "    -w      Median Filter Window Size (3, 5, 7, ...)" << std::endl;
        std::cout << "    -t      Difference Threshold (0..1)" << std::endl;

        return;
    }

    //----------------------------------------------------------------------
    template<typename T>
    T absolute_difference(const T& a, const T& b)
    {
        return abs(a - b);
    }

    //----------------------------------------------------------------------
    quetzal::gio::BitMap::pixel_type to_color(const Matrix::element_type& e)
    {
        return quetzal::Color(e, e, e).packed();
    }

    //----------------------------------------------------------------------
    class Highlight
    {
    public:

        //--------------------------------------------------------------
        Highlight(const Matrix::element_type& threshold = 0.1, quetzal::gio::BitMap::pixel_type color = 0xffff00ff) :
            m_threshold(threshold),
            m_color(color)
        {
        }

        //--------------------------------------------------------------
        quetzal::gio::BitMap::pixel_type operator()(const quetzal::gio::BitMap::pixel_type& c, const Matrix::element_type& e)
        {
            if (e > m_threshold)
            {
                return m_color;
            }

            return c;
        }

    private:

        Matrix::element_type m_threshold;
        quetzal::gio::BitMap::pixel_type m_color;
    };

    //----------------------------------------------------------------------
    Matrix::element_type median(const Matrix::element_type* pe, int stride, int sizeWindow)
    {
        std::vector<Matrix::element_type> elements(sizeWindow * sizeWindow);
        int sizeWindow2 = sizeWindow / 2;

        for (int j = 0; j < sizeWindow; ++j)
        {
            for (int i = 0; i < sizeWindow; ++i)
            {
                elements[j * sizeWindow + i] = pe[(j - sizeWindow2) * stride + (i - sizeWindow2)];
            }
        }

        std::sort(elements.begin(), elements.end());

        return elements[elements.size() / 2];
    }

    //----------------------------------------------------------------------
    void median_filter(const Matrix& mIn, Matrix& mOut, int sizeWindow)
    {
        assert(sizeWindow > 0 && (sizeWindow & 1) != 0);
        assert(mIn.data().size() == mOut.data().size());

        int w = mIn.sizei();
        int h = mIn.sizej();
        int border = sizeWindow / 2;

        const Matrix::array_type& dataIn = mIn.data();
        const Matrix::element_type* const pIn0 = &dataIn[0];

        Matrix::array_type& dataOut = mOut.data();
        Matrix::element_type* const pOut0 = &dataOut[0];

        // Copy border elements

        for (int j = 0; j < border; ++j)
        {
            memcpy(&pOut0[j * w], &pIn0[j * w], w * sizeof (Matrix::element_type)); // top
            memcpy(&pOut0[(h - 1 - j) * w], &pIn0[(h - 1 - j) * w], w * sizeof (Matrix::element_type)); // bottom
        }

        const Matrix::element_type* pRowIn = &pIn0[w * border];
        Matrix::element_type* pRowOut = &pOut0[w * border];

        for (int j = border; j < h - border; ++j)
        {
            for (int i = 0; i < border; ++i)
            {
                pRowOut[i] = pRowIn[i]; // left
                pRowOut[w - 1 - i] = pRowIn[w - 1 - i]; // right
            }

            pRowIn += w;
            pRowOut += w;
        }

        // Calculate internal elements

        pRowIn = &pIn0[w * border + border];
        pRowOut = &pOut0[w * border + border];

        for (int j = border; j < h - border; ++j)
        {
            const Matrix::element_type* pIn = pRowIn;
            Matrix::element_type* pOut = pRowOut;

            for (int i = border; i < w - border; ++i)
            {
                *pOut++ = median(pIn++, w, sizeWindow);
            }

            pRowIn += w;
            pRowOut += w;
        }

        return;
    }

    //----------------------------------------------------------------------
    void test_transform(const quetzal::gio::BitMap& bmIn, quetzal::gio::BitMap& bmOut1, quetzal::gio::BitMap& bmOut2, int sizeWindow, float threshold)
    {
        Matrix mGray(bmIn.width(), bmIn.height());

        Matrix::array_type::iterator iGray = mGray.data().begin();

        quetzal::gio::BitMap::array_type::const_iterator iInEnd = bmIn.data().end();
        for (quetzal::gio::BitMap::array_type::const_iterator iIn = bmIn.data().begin(); iIn != iInEnd; ++iIn)
        {
            *iGray++ = quetzal::Color(*iIn).y();
        }

        Matrix mDetailless(mGray.sizei(), mGray.sizej());
        median_filter(mGray, mDetailless, sizeWindow);

        Matrix mDifference(mGray.sizei(), mGray.sizej());
        std::transform(mGray.data().begin(), mGray.data().end(), mDetailless.data().begin(), mDifference.data().begin(), absolute_difference<Matrix::element_type>);
        
        // Apply intermediate results to bmOut1

        bmOut1.resize(bmIn.width(), bmIn.height());
        std::transform(mGray.data().begin(), mGray.data().end(), bmOut1.data().begin(), to_color);

//        std::transform(mDetailless.data().begin(), mDetailless.data().end(), bmOut1.data().begin(), to_color);
//        std::transform(mDifference.data().begin(), mDifference.data().end(), bmOut1.data().begin(), to_color);
        Highlight highlight(threshold, 0xffff00ff);
        std::transform(bmIn.data().begin(), bmIn.data().end(), mDifference.data().begin(), bmOut1.data().begin(), highlight);

        // Apply corrections to bmOut2

        bmOut2 = bmIn;

        // where difference > threshold, blur from original ...
    }

} // namespace

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    // Parse arguments

    quetzal::Arguments args;
    args.register_argument(ArgMedianWindow, "", "-w", 1, true, "3", "3");
    args.register_argument(ArgThreshold, "", "-t", 1, true, ".1", ".1");

    args.parse(argc, argv);

//    std::cout << args << std::endl;

    if (args.positional() != 3)
    {
        usage();
        return 1;
    }

    try
    {
        std::string fileIn = args.positional_value(0);
        quetzal::io::InputFileStream input(fileIn);
        if (!input)
        {
            std::cout << "Error opening input file: " << fileIn << std::endl;
            return 1;
        }

        std::string formatId = quetzal::file_extension(fileIn); // May need more complex processing? ...

        quetzal::gio::BitMap bmIn;
        quetzal::gio::read(input, bmIn, formatId);

        quetzal::gio::BitMap bmOut1;
        quetzal::gio::BitMap bmOut2;

        test_transform(bmIn, bmOut1, bmOut2, args.named_value<int>(ArgMedianWindow), args.named_value<float>(ArgThreshold));

        // Write intermediate file

        std::string fileOut1 = args.positional_value(1);
        quetzal::io::OutputFileStream output1(fileOut1);
        if (!output1)
        {
            std::cout << "Error opening output file: " << fileOut1 << std::endl;
            return 1;
        }

        quetzal::gio::write(output1, bmOut1, formatId);

        // Write output file

        std::string fileOut2 = args.positional_value(2);
        quetzal::io::OutputFileStream output2(fileOut2);
        if (!output2)
        {
            std::cout << "Error opening output file: " << fileOut2 << std::endl;
            return 1;
        }

        quetzal::gio::write(output2, bmOut2, formatId);
    }
    catch (quetzal::Exception& e)
    {
        std::cout << e.what() << " at " << e.file() << " line " << e.line() << std::endl;
    }

    return 0;
}
