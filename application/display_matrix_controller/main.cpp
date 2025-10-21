//------------------------------------------------------------------------------
// Display Matrix Controller
// main.cpp
//
//------------------------------------------------------------------------------

#include "quetzal/serial/Serial.hpp"
#include "quetzal/serial/Exception.hpp"
#include "quetzal/string_util.hpp"
#include "quetzal/Timestamp.hpp"
#include <vector>
#include <iostream>
#include <iomanip>
#include <cassert>

#include <fstream>

using namespace std;

namespace
{

    const string strPortDefault("COM9");
    const unsigned long baudrateDefault(115200);

    // Frame rate
//    const double rate = 30.;
    const double rate = 2.;
    const double period = 1 / rate;

    // Per device
    const int nDeviceChannels = 16;
    const int nChannelBitsGS = 12;
    const int nChannelBitsDC = 6;
    const int nDeviceBitsStatus = 192;

    // Per strip
    const int nStripDevices = 3;

    // Per frame
    //const int nStrips = 16;
    const int nStrips = 1;
    const int nDevices = nStrips * nStripDevices;
    const int nChannels = nDevices * nDeviceChannels;
    const int nBytesGS = nChannels * nChannelBitsGS / 8;
    const int nBytesDC = nChannels * nChannelBitsDC / 8;

    const size_t CommandSize = 8;
    const char commandGS[] = "Csync_01";
    const char commandDC1[] = "Csync_02";
    const char commandDC2[] = "Csync_03"; // Also write DC data to EEPROM
    const char commandStatus[] = "Cstat_04"; // Request status information
    const char commandError[] = "Cstat_05"; // Request XERR status

    const char commandStatus2[] = "Cstat_06"; // Request status information

    // 1 byte per channel
    const char s_dataDC[nChannels] =
    {
//        '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff',
//        '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff',
//        '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff',
        '\x00', '\x11', '\x22', '\x33', '\x44', '\x55', '\x66', '\x77', '\x88', '\x99', '\xaa', '\xbb', '\xcc', '\xdd', '\xee', '\xff',
        '\x00', '\x11', '\x22', '\x33', '\x44', '\x55', '\x66', '\x77', '\x88', '\x99', '\xaa', '\xbb', '\xcc', '\xdd', '\xee', '\xff',
        '\x00', '\x11', '\x22', '\x33', '\x44', '\x55', '\x66', '\x77', '\x88', '\x99', '\xaa', '\xbb', '\xcc', '\xdd', '\xee', '\xff',
    };

    // test frame data ...
    const int nFrames = 10;

    // 1 byte per channel
    const char s_dataGS[nFrames * nChannels] =
    {
        '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
        '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
        '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',

        '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
        '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
        '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff',

        '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
        '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20',
        '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff',

        '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
        '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f',
        '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f',

        '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
        '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff',
        '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',

        '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f',
        '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f',
        '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',

        '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff',
        '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
        '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',

        '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff', '\xff',
        '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
        '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20', '\x20',

        '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f',
        '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00',
        '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f', '\x7f',

        '\x55', '\x55', '\x55', '\x55', '\x55', '\x55', '\x55', '\x55', '\x55', '\x55', '\x55', '\x55', '\x55', '\x55', '\x55', '\x55',
        '\x55', '\x55', '\x55', '\x55', '\x55', '\x55', '\x55', '\x55', '\x55', '\x55', '\x55', '\x55', '\x55', '\x55', '\x55', '\x55',
        '\x55', '\x55', '\x55', '\x55', '\x55', '\x55', '\x55', '\x55', '\x55', '\x55', '\x55', '\x55', '\x55', '\x55', '\x55', '\x55',
    };

} // namespace

//------------------------------------------------------------------------------
void read_dc(vector<unsigned char>& dc)
{
    assert(dc.size() == nBytesDC);

    int iRaw = 0;

    // Every 4 input bytes produce 3 packed output bytes
    for (int iPacked = 0; iPacked < nBytesDC; )
    {
        dc[iPacked++] = (s_dataDC[iRaw] << 2) | (s_dataDC[iRaw + 1] >> 4);
        dc[iPacked++] = (s_dataDC[iRaw + 1] << 4) | (s_dataDC[iRaw + 2] >> 2);
        dc[iPacked++] = (s_dataDC[iRaw + 2] << 6) | s_dataDC[iRaw + 3];

        iRaw += 4;
    }

    return;
}

//------------------------------------------------------------------------------
void update_frame(vector<unsigned char>& frame)
{
    assert(frame.size() == nBytesGS);

    static int nFrame = 0;

    const char* pdata = &s_dataGS[nFrame * nChannels];

    int iRaw = 0;

    // Every 2 input bytes produce 3 packed output bytes
    for (int iPacked = 0; iPacked < nBytesGS; )
    {
        frame[iPacked++] = pdata[iRaw];
        frame[iPacked++] = (pdata[iRaw] & 0xf0) | (pdata[iRaw + 1] >> 4);
        frame[iPacked++] = (pdata[iRaw + 1] << 4) | (pdata[iRaw + 1] >> 4);

        iRaw += 2;
    }

    nFrame = ++nFrame % nFrames;

    return;
}

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    string strPort(strPortDefault);
    unsigned long baudrate(baudrateDefault);

    if (argc > 2)
    {
        baudrate = quetzal::to_type<unsigned long>(argv[2]);
    }

    if (argc > 1)
    {
        strPort = argv[1];
    }

    vector<unsigned char> dc(nBytesDC);
    vector<unsigned char> gs(nBytesGS);

//    ofstream os("data.txt");
    ostream& os = cout;

    try
    {
        quetzal::serial::Serial serial;
        quetzal::Timestamp tsNext;

        for (;;)
        {
            try
            {
                if (!serial.is_open())
                {
                    serial.open(strPort, baudrate);
                    continue;
                }

                cout << "Serial connected: " << strPort << " " << baudrate << endl;
/*
                // Write dot correction data

                read_dc(dc); // Read stored DC data

                size_t size = serial.write(commandDC1, CommandSize);
                assert(size == CommandSize);

//os << "DC" << endl;

//     just write all dc data at once here? ...
//                size = serial.write(dc.data(), dc.size());
//                assert(size == dc.size());
                for (size_t i = 0; i < dc.size(); ++i)
                {
//os << i << " " << int(dc[i]) << endl;
                    size = serial.write(&dc[i], 1);
                    assert(size == 1);
                }
*/
size_t size;

                // Write grayscale data

                for (; ; )
                {
                    update_frame(gs); // Read next frame data

                    Sleep(500); // just until next frame period ...
//                    tsNext.sleep_until();
//                    tsNext += period;
//                    tsNext = Timestamp() + period;

                    size = serial.write(commandGS, CommandSize);
                    assert(size == CommandSize);

// just write all gs data at once here? should be ok on this end unless we need to interleave to get more time for frame update ...
//                    size = serial.write(gs.data(), gs.size());
//                    assert(size == gs.size());
                    for (size_t j = 0; j < gs.size(); ++j)
                    {
                        size = serial.write(&gs[j], 1);
                        assert(size == 1);

                        while (serial.available()) // Read returned status information
                        {
//                            unsigned char c;
//                            serial.read(&c, 1);
//                            os << setw(2) << setfill('0') << hex << int(c) << endl;
                            char c;
                            serial.read(&c, 1);
                            os << c;
//                            os << flush;
                        }
                    }

//                    size = serial.write(commandStatus, CommandSize);
//                    size = serial.write(commandStatus2, CommandSize);
//                    assert(size == CommandSize);
                }
            }
            catch (quetzal::serial::Exception&) // Serial disconnection, wait and try to reconnect on next loop iteration
            {
                Sleep(100);
            }
            catch (...) // Rethrow any other exceptions
            {
                throw;
            }
        }
    }
    catch (exception& e)
    {
        cout << e.what() << endl;
    }

    return 0;
}
