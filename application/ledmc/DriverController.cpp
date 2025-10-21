//------------------------------------------------------------------------------
// DriverController.cpp
//------------------------------------------------------------------------------

#include "DriverController.hpp"
#include "debug_assert.hpp"
#include <iostream>

using namespace std;

//------------------------------------------------------------------------------
quetzal::DriverController::DriverController(const SystemConfiguration& system_config) :
    m_system_config(system_config),
    m_thread(),
    m_mutex(),
    m_bStop(false),
//    m_nStrips(m_system_config.strips()),
m_nStrips(2), // ...
    m_nDeviceBitsGS(m_system_config.device_channels() * m_system_config.channel_bits_gs()),
    m_nDeviceBitsDC(m_system_config.device_channels() * m_system_config.channel_bits_dc()),
    m_nStripBitsGS(m_system_config.strip_devices() * m_nDeviceBitsGS),
    m_nStripBitsDC(m_system_config.strip_devices() * m_nDeviceBitsDC),
    m_nStripBitsStatus(m_system_config.strip_devices() * m_system_config.device_bits_status()),
    m_nBitsGS(m_nStrips * m_nStripBitsGS),
    m_nBitsDC(m_nStrips * m_nStripBitsDC),
    m_nBitsStatus(m_nStrips * m_nStripBitsStatus),
    m_nBytesGS(m_nBitsGS / 8),
    m_nBytesDC(m_nBitsDC / 8),
    m_nBytesStatus(m_nBitsStatus / 8),
    m_nCyclesGS(1 << m_system_config.channel_bits_gs()),
    m_out_x_sin(m_system_config.gpio_ser()),
    m_out_x_sclk(m_system_config.gpio_srclk()),
    m_out_x_latch(m_system_config.gpio_rclk()),
    m_out_x_enable(m_system_config.gpio_oe()),
    m_out_y_sin(m_system_config.gpio_sin()),
    m_out_y_latch(m_system_config.gpio_xlat()),
    m_out_y_blank(m_system_config.gpio_blank()),
    m_out_y_gsclk(m_system_config.gpio_gsclk()),
    m_out_y_vprg(m_system_config.gpio_vprg()),
    m_out_y_dcprg(m_system_config.gpio_dcprg()),
    m_in_sout(m_system_config.gpio_sout()),
    m_in_error(m_system_config.gpio_xerr()),
    m_bufferGS0(m_nBytesGS),
    m_bufferGS1(m_nBytesGS),
    m_bufferGS2(m_nBytesGS),
    m_pbufferGSBack(&m_bufferGS0),
    m_pbufferGSMid(&m_bufferGS1),
    m_pbufferGSFront(&m_bufferGS2),
    m_bGSAvailable(false),
    m_bufferDC(m_nBytesDC),
    m_bDC(false),
    m_bFirstCycle(true),
    m_bufferStatus(m_nBytesStatus),
    m_bPushStatus(false),
    m_bPopStatus(false),
    m_bError(false)
{
cout << "m_nStrips " << m_nStrips << endl;
cout << "m_nDeviceBitsGS " << m_nDeviceBitsGS << endl;
cout << "m_nDeviceBitsDC " << m_nDeviceBitsDC << endl;
cout << "m_nStripBitsGS " << m_nStripBitsGS << endl;
cout << "m_nStripBitsDC " << m_nStripBitsDC << endl;
cout << "m_nStripBitsStatus " << m_nStripBitsStatus << endl;
cout << "m_nBitsGS " << m_nBitsGS << endl;
cout << "m_nBitsDC " << m_nBitsDC << endl;
cout << "m_nBitsStatus " << m_nBitsStatus << endl;
cout << "m_nBytesGS " << m_nBytesGS << endl;
cout << "m_nBytesDC " << m_nBytesDC << endl;
cout << "m_nBytesStatus " << m_nBytesStatus << endl;
cout << "m_nCyclesGS " << m_nCyclesGS << endl;

    m_thread = thread(ref(*this));
}

//------------------------------------------------------------------------------
quetzal::DriverController::~DriverController()
{
    stop();
    m_thread.join();
}

//------------------------------------------------------------------------------
void quetzal::DriverController::operator()()
{
    for (;;)
    {
        if (m_bStop)
        {
            break;
        }

        if (m_bPushStatus)
        {
            init_load_status();
            set_strip(0);

            for (int i = 0; i < m_nBitsStatus;)
            {
                m_out_y_sin.write_high();
                m_out_x_enable.pulse_high(); // y_sclk

                ++i;
                if ((i % m_nStripBitsStatus) == 0)
                {
                    next_strip();
                }
            }

            final_load_status();
            m_bPushStatus = false;
            m_bPopStatus = true;
            continue;
        }

        if (m_bDC)
        {
            init_load_dc();
            set_strip(0);
            const buffer_data_type* p = m_bufferDC.data();
            unsigned int byte_mask = 0x80;

            for (int i = 0; i < m_nBitsDC;)
            {
                m_out_y_sin.write(*p & byte_mask);
                m_out_x_enable.pulse_high(); // y_sclk

                byte_mask >>= 1;
                if (byte_mask == 0)
                {
                    ++p;
                    byte_mask = 0x80;
                }

                ++i;
                if ((i % m_nStripBitsDC) == 0)
                {
                    next_strip();
                }
            }

            final_load_dc();
            m_bDC = false;
            continue;
        }

        // Grayscale

        init_load_gs();
        set_strip(0);
        if (m_bGSAvailable)
        {
            unique_lock<mutex> lock(m_mutex);
            swap(m_pbufferGSFront, m_pbufferGSMid);
            m_bGSAvailable = false;
        }
        const buffer_data_type* p = m_pbufferGSFront->data();
        unsigned int byte_mask = 0x80;
        int nBitGS = 0;

        unsigned char status_byte = 0; // status bit accumulator
        int nByteStatus = 0;

        for (int i = 0; i < m_nCyclesGS; ++i)
        {
// May need to do this multiple times ...
// divide bits evenly between gs cycles
// calculate nBitGS for current nCycleGS and subtract prev
// write that many bits
            int nBit = (m_nBitsGS * (i + 1) + m_nCyclesGS / 2) / m_nCyclesGS;
            while (nBitGS < nBit)
//            if (nBitGS < m_nBitsGS)
            {
                m_out_y_sin.write(*p & byte_mask);
                m_out_x_enable.pulse_high(); // y_sclk

                if (m_bPopStatus)
                {
                    if (m_in_sout.read())
                    {
                        status_byte |= byte_mask;
                    }

                    if (byte_mask == 0x01) // Last bit for this byte
                    {
                        m_bufferStatus[nByteStatus++] = status_byte;
                        status_byte = 0;
                    }

                    if (nByteStatus == m_nBytesStatus)
                    {
                        m_bPopStatus = false;
                    }
                }

                byte_mask >>= 1;
                if (byte_mask == 0)
                {
                    ++p;
                    byte_mask = 0x80;
                }

                ++nBitGS;
                if ((nBitGS % m_nStripBitsGS) == 0)
                {
                    next_strip();
                }
            }

            if (i == 16) // Sample state of XERR (arbitrary delay after start of GS cycle ...)
            {
                m_bError = m_in_error.read();
            }

            m_out_y_gsclk.pulse_high();
        }

        final_load_gs();
    }

    return;
}

//------------------------------------------------------------------------------
void quetzal::DriverController::stop()
{
    m_bStop = true;
    return;
}

//------------------------------------------------------------------------------
const quetzal::SystemConfiguration& quetzal::DriverController::system_config() const
{
    return m_system_config;
}

//------------------------------------------------------------------------------
int quetzal::DriverController::width() const
{
    return m_nStrips;
}

//------------------------------------------------------------------------------
int quetzal::DriverController::height() const
{
    return m_system_config.device_channels();
}

//------------------------------------------------------------------------------
int quetzal::DriverController::components() const
{
    return m_system_config.strip_devices();
}

//------------------------------------------------------------------------------
size_t quetzal::DriverController::dc_buffer_size() const
{
    return m_nBytesDC;
}

//------------------------------------------------------------------------------
size_t quetzal::DriverController::gs_buffer_size() const
{
    return m_nBytesGS;
}

//------------------------------------------------------------------------------
void quetzal::DriverController::set_dc(const buffer_type& buffer)
{
    debug_assert(buffer.size() == dc_buffer_size());
    debug_assert(!m_bDC);

    m_bufferDC.assign(buffer.begin(), buffer.end()); // swap or move ...
    m_bDC = true;
    return;
}

//------------------------------------------------------------------------------
void quetzal::DriverController::set_gs(const buffer_type& buffer)
{
    debug_assert(buffer.size() == gs_buffer_size());
    debug_assert(!m_bGSAvailable);

    m_pbufferGSBack->assign(buffer.begin(), buffer.end());

    auto& p = m_pbufferGSBack;
    m_pbufferGSBack = const_cast<buffer_type*>(m_pbufferGSMid);
    unique_lock<mutex> lock(m_mutex);
    m_pbufferGSMid = p;

    m_bGSAvailable = true; 
    return;
}

//------------------------------------------------------------------------------
void quetzal::DriverController::init_status()
{
    m_bPushStatus = true;
    return;
}

//------------------------------------------------------------------------------
bool quetzal::DriverController::get_status(buffer_type& buffer)
{
    if (m_bPushStatus || m_bPopStatus)
    {
        return false; // Data not yet available
    }

    buffer.assign(m_bufferStatus.begin(), m_bufferStatus.end());
    return true;
}

//------------------------------------------------------------------------------
bool quetzal::DriverController::error_state()
{
    bool bError = m_bError;
    m_bError = false;
    return bError;
}

//------------------------------------------------------------------------------
void quetzal::DriverController::set_strip(int n)
{
    for (int i = 0; i < m_nStrips; ++i)
    {
        m_out_x_sin.write(n == m_nStrips - i - 1);
        m_out_x_sclk.pulse_high();
    }

    m_out_x_latch.pulse_high();
    return;
}

//------------------------------------------------------------------------------
void quetzal::DriverController::next_strip()
{
    m_out_x_sclk.pulse_high();
    m_out_x_latch.pulse_high();
    return;
}

//------------------------------------------------------------------------------
void quetzal::DriverController::init_load_gs()
{
    if (m_bFirstCycle)
    {
        m_out_y_vprg.write_low();
    }

    m_out_y_blank.write_low();

    return;
}

//------------------------------------------------------------------------------
void quetzal::DriverController::final_load_gs()
{
    m_out_y_blank.write_high();
    m_out_y_latch.pulse_high();

    if (m_bFirstCycle)
    {
        set_strip(0);
        for (int i = 0; i < m_nStrips; ++i)
        {
            m_out_x_enable.pulse_high(); // y_sclk
            next_strip();
        }

        m_bFirstCycle = false;
    }

    return;
}

//------------------------------------------------------------------------------
void quetzal::DriverController::init_load_dc()
{
    m_out_y_dcprg.write_high();
    m_out_y_vprg.write_high();
    return;
}

//------------------------------------------------------------------------------
void quetzal::DriverController::final_load_dc()
{
    m_out_y_latch.pulse_high();
    m_bFirstCycle = true;
    return;
}

//------------------------------------------------------------------------------
void quetzal::DriverController::init_load_status()
{
    return;
}

//------------------------------------------------------------------------------
void quetzal::DriverController::final_load_status()
{
    m_out_y_latch.write_high();
    m_out_y_blank.write_low();
    m_out_y_gsclk.pulse_high();
    this_thread::sleep_for(chrono::duration<double>(.000002));
    m_out_y_latch.write_low();
    m_out_y_blank.write_high();
    return;
}
