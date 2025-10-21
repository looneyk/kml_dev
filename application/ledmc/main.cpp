//------------------------------------------------------------------------------
// main.cpp
//------------------------------------------------------------------------------

#include "DriverController.hpp"
#include "Frame.hpp"
#include "debug_assert.hpp"
#include "quetzal/Timestamp.hpp"
#include "quetzal/Exception.hpp"
#include "quetzal/pi_gpio/pi_gpio.hpp"
#include <iostream>
//#include <array>
#include <cstring>
#include <unistd.h>

//#include <chrono> // ...
#include <iomanip> // ...

using namespace std;

namespace
{

    // Frame rate
    const double rate = 30.;
//    const double rate = 2.;
    const double period = 1 / rate;

const int s_nSteps = 15;
int s_nStep = 0;

    const string s_system_configuration_filename = "system_configuration.xml";

    // Per device
    const int s_nDeviceChannels = 16;

    // Per strip
    const int s_nStripDevices = 3;

    // Per frame
    //const int s_nStrips = 16;
const int s_nStrips = 2;
    const int s_nDevices = s_nStrips * s_nStripDevices;
    const int s_nChannels = s_nDevices * s_nDeviceChannels;

    // test DC data ...
    const int s_nStripChannels = s_nDevices * s_nDeviceChannels;

    // test GS frame data ...
const int s_nFrames = 8;

const quetzal::DriverController::buffer_data_type cx0 = 0x00;
const quetzal::DriverController::buffer_data_type c0r = 0x61;
const quetzal::DriverController::buffer_data_type c1r = 0x4e;
const quetzal::DriverController::buffer_data_type c1g = 0x09;
const quetzal::DriverController::buffer_data_type c2r = 0x20;
const quetzal::DriverController::buffer_data_type c2g = 0x20;
const quetzal::DriverController::buffer_data_type c3g = 0x31;
const quetzal::DriverController::buffer_data_type c4g = 0x29;
const quetzal::DriverController::buffer_data_type c4b = 0x29;
const quetzal::DriverController::buffer_data_type c5b = 0xff;
const quetzal::DriverController::buffer_data_type c6r = 0x18;
const quetzal::DriverController::buffer_data_type c6b = 0xc0;
const quetzal::DriverController::buffer_data_type c7r = 0x46;
const quetzal::DriverController::buffer_data_type c7b = 0x46;

    // 1 byte per channel
//    quetzal::DriverController::buffer_data_type s_dataGS[s_nFrames * s_nChannels] =
quetzal::DriverController::buffer_data_type s_dataGSStrip[s_nFrames * s_nStripChannels] =
{
    cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0,
    cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0,
    c0r, c0r, c0r, c0r, c0r, c0r, c0r, c0r, c0r, c0r, c0r, c0r, c0r, c0r, c0r, c0r,

    cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0,
    c1g, c1g, c1g, c1g, c1g, c1g, c1g, c1g, c1g, c1g, c1g, c1g, c1g, c1g, c1g, c1g,
    c1r, c1r, c1r, c1r, c1r, c1r, c1r, c1r, c1r, c1r, c1r, c1r, c1r, c1r, c1r, c1r,

    cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0,
    c2g, c2g, c2g, c2g, c2g, c2g, c2g, c2g, c2g, c2g, c2g, c2g, c2g, c2g, c2g, c2g,
    c2r, c2r, c2r, c2r, c2r, c2r, c2r, c2r, c2r, c2r, c2r, c2r, c2r, c2r, c2r, c2r,

    cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0,
    c3g, c3g, c3g, c3g, c3g, c3g, c3g, c3g, c3g, c3g, c3g, c3g, c3g, c3g, c3g, c3g,
    cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0,

    c4b, c4b, c4b, c4b, c4b, c4b, c4b, c4b, c4b, c4b, c4b, c4b, c4b, c4b, c4b, c4b,
    c4g, c4g, c4g, c4g, c4g, c4g, c4g, c4g, c4g, c4g, c4g, c4g, c4g, c4g, c4g, c4g,
    cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0,

    c5b, c5b, c5b, c5b, c5b, c5b, c5b, c5b, c5b, c5b, c5b, c5b, c5b, c5b, c5b, c5b,
    cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0,
    cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0,

    c6b, c6b, c6b, c6b, c6b, c6b, c6b, c6b, c6b, c6b, c6b, c6b, c6b, c6b, c6b, c6b,
    cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0,
    c6r, c6r, c6r, c6r, c6r, c6r, c6r, c6r, c6r, c6r, c6r, c6r, c6r, c6r, c6r, c6r,

    c7b, c7b, c7b, c7b, c7b, c7b, c7b, c7b, c7b, c7b, c7b, c7b, c7b, c7b, c7b, c7b,
    cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0, cx0,
    c7r, c7r, c7r, c7r, c7r, c7r, c7r, c7r, c7r, c7r, c7r, c7r, c7r, c7r, c7r, c7r,
};

//    const int s_nFrames = s_dataGSStrip::size() / s_nStripChannels;

    // 1 byte per channel
    quetzal::DriverController::buffer_data_type s_dataGS[s_nFrames * s_nChannels];

} // namespace

//------------------------------------------------------------------------------
void pack_frame_6(const quetzal::Frame& frame, quetzal::DriverController::buffer_type& buffer)
{
    int componentMax = 64; // 1 << system_config.channel_bits_dc();
    int nChannels = frame.height() * frame.width() * frame.components();

    quetzal::DriverController::buffer_data_type components[nChannels];

    // Reduce range to 0..componentMax
    for (int i = 0; i < nChannels; ++i)
    {
        components[i] = frame.data()[i] * componentMax;
        if (components[i] >= componentMax)
        {
            components[i] = componentMax - 1;
        }
    }

    // Every 4 input bytes produce 3 packed output bytes
    size_t iRaw = 0;
    for (size_t iPacked = 0; iPacked < buffer.size(); )
    {
        buffer[iPacked++] = (components[iRaw] << 2) | (components[iRaw + 1] >> 4);
        buffer[iPacked++] = (components[iRaw + 1] << 4) | (components[iRaw + 2] >> 2);
        buffer[iPacked++] = (components[iRaw + 2] << 6) | components[iRaw + 3];

        iRaw += 4;
    }

    return;
}

//------------------------------------------------------------------------------
void pack_frame_12(const quetzal::Frame& frame, quetzal::DriverController::buffer_type& buffer)
{
    int componentMax = 4096; // 1 << system_config.channel_bits_gs();
    int nChannels = frame.height() * frame.width() * frame.components();

    quetzal::DriverController::buffer_data_type components[nChannels];

    // Reduce range to 0..componentMax
    for (int i = 0; i < nChannels; ++i)
    {
        components[i] = frame.data()[i] * componentMax;
        if (components[i] >= componentMax)
        {
            components[i] = componentMax - 1;
        }
    }

    // Every 2 input bytes produce 3 packed output bytes
    size_t iRaw = 0;
    for (size_t iPacked = 0; iPacked < buffer.size(); )
    {
        buffer[iPacked++] = components[iRaw];
        buffer[iPacked++] = (components[iRaw] & 0xf0) | (components[iRaw + 1] >> 4);
        buffer[iPacked++] = ((components[iRaw + 1] << 4) & 0xf0) | (components[iRaw + 1] >> 4);

        iRaw += 2;
    }

    return;
}

//------------------------------------------------------------------------------
void load_dc(const string& filename, quetzal::DriverController::buffer_type& buffer)
{
    quetzal::Frame frame;
    frame.load(filename);
cout << frame.width() << " " << frame.height() << " " << frame.components() << " " << endl;
cout << buffer.size() << endl;

    debug_assert(frame.components() == 1);
    debug_assert(static_cast<size_t>(frame.width() * frame.height() * frame.components()) == buffer.size());

    pack_frame_6(frame, buffer);
    return;
}

//------------------------------------------------------------------------------
void load_frame(const string& filename, quetzal::DriverController::buffer_type& buffer)
{
    quetzal::Frame frame;
    frame.load(filename);

    debug_assert(frame.components() == 3);
    debug_assert(static_cast<size_t>(frame.width() * frame.height() * frame.components()) == buffer.size());

    pack_frame_12(frame, buffer);
    return;
}

//------------------------------------------------------------------------------
void check_input()
{
    // if kbhit getc, if peek get, ...
    // update pattern ...

    return;
}

//------------------------------------------------------------------------------
void update_frame(quetzal::DriverController& controller, quetzal::DriverController::buffer_type& buffer)
{
// duplicate strip data for all strips ...
// really calculate frame data ...
int nStripChannels = controller.height() * controller.components();
quetzal::DriverController::buffer_data_type* pdataIn = s_dataGSStrip;
quetzal::DriverController::buffer_data_type* pdataOut = s_dataGS;
for (int j = 0; j < s_nFrames; ++j)
{
    for (int i = 0; i < controller.width(); ++i)
    {
        memcpy(pdataOut, pdataIn, nStripChannels * sizeof (quetzal::DriverController::buffer_data_type));
        pdataOut += nStripChannels;
    }
    pdataIn += nStripChannels;
}

    static int nFrame = 0;

    int nChannels = controller.width() * controller.height() * controller.components();

    const quetzal::DriverController::buffer_data_type* pdata0 = &s_dataGS[nFrame * nChannels];
    const quetzal::DriverController::buffer_data_type* pdata1 = (nFrame == s_nFrames - 1 ? &s_dataGS[0] : &s_dataGS[(nFrame + 1) * nChannels]);

    quetzal::DriverController::buffer_type pdata(buffer.size());
    for (size_t i = 0; i < buffer.size(); ++i)
    {
        pdata[i] = (pdata0[i] * double(s_nSteps - s_nStep) + pdata1[i] * double(s_nStep)) / s_nSteps; // round
    }

    // Every 2 input bytes produce 3 packed output bytes
    size_t iRaw = 0;
    for (size_t iPacked = 0; iPacked < buffer.size(); )
    {
        buffer[iPacked++] = pdata[iRaw];
        buffer[iPacked++] = (pdata[iRaw] & 0xf0) | (pdata[iRaw + 1] >> 4);
        buffer[iPacked++] = ((pdata[iRaw + 1] << 4) & 0xf0) | (pdata[iRaw + 1] >> 4);

        iRaw += 2;
    }

    if (++s_nStep >= s_nSteps)
    {
        ++nFrame;
        s_nStep = 0;
    }

    if (nFrame >= s_nFrames)
    {
        nFrame = 0;
    }

    return;
}

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    quetzal::pi_gpio::init();
    setuid(getuid()); // Return to normal user privileges

    try
    {
        quetzal::SystemConfiguration system_config;
        system_config.load(s_system_configuration_filename);
//cout << system_config << endl;

        quetzal::DriverController controller(system_config);

        quetzal::DriverController::buffer_type bufferDC(controller.dc_buffer_size());
        quetzal::DriverController::buffer_type bufferGS(controller.gs_buffer_size());

        // Write dot correction data

        load_dc(system_config.dot_correction_filename(), bufferDC); // Read stored DC data
        controller.set_dc(bufferDC);

        quetzal::Timestamp tsNext;

        for (int nFrame = 0; ; ++nFrame)
        {
            check_input();

            // Update, next frame data
            update_frame(controller, bufferGS);
            cout << "Frame " << nFrame << "\r" << flush;
            tsNext.sleep_until(); // fixed or free running? ....

            // Render, write grayscale data
            controller.set_gs(bufferGS);

            tsNext += period;
            if (tsNext < quetzal::Timestamp()) // If it has already passed, do it as soon as possible
            {
                tsNext.set_now();
            }
        }
    }
    catch (exception& e)
    {
        cout << e.what() << endl;
    }

    return 0;
}
