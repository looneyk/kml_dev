#if !defined(QUETZAL_DRIVERCONTROLLER_HPP)
#define QUETZAL_DRIVERCONTROLLER_HPP
//------------------------------------------------------------------------------
// DriverController.hpp
//------------------------------------------------------------------------------

#include "quetzal/pi_gpio/PinOutput.hpp"
#include "quetzal/pi_gpio/PinInput.hpp"
#include "SystemConfiguration.hpp"
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>

namespace quetzal
{

    //--------------------------------------------------------------------------
    class DriverController
    {
    public:

        typedef unsigned char buffer_data_type;
        typedef std::vector<buffer_data_type> buffer_type;

        DriverController(const SystemConfiguration& system_config);
        DriverController(const DriverController&) = delete;
        ~DriverController();

        DriverController& operator=(const DriverController&) = delete;

        void operator()();

        void stop();

        const SystemConfiguration& system_config() const;

        int width() const;
        int height() const;
        int components() const;

        size_t dc_buffer_size() const;
        size_t gs_buffer_size() const;

        void set_dc(const buffer_type& buffer); // Must contain dc_buffer_size() elements
        void set_gs(const buffer_type& buffer); // Must contain gs_buffer_size() elements

        void init_status(); // Start loading status bits from drivers
        bool get_status(buffer_type& buffer);

        bool error_state(); // Get and clear saved error state

    private:

        void set_strip(int n);
        void next_strip();

        void init_load_gs();
        void final_load_gs();
        void init_load_dc();
        void final_load_dc();
        void init_load_status();
        void final_load_status();

        SystemConfiguration m_system_config;

        std::thread m_thread;
        std::mutex m_mutex; // Control access to m_pbufferGSMid

        std::atomic<bool> m_bStop;

        int m_nStrips;
        int m_nDeviceBitsGS;
        int m_nDeviceBitsDC;
        int m_nStripBitsGS;
        int m_nStripBitsDC;
        int m_nStripBitsStatus;
        int m_nBitsGS;
        int m_nBitsDC;
        int m_nBitsStatus;
        int m_nBytesGS;
        int m_nBytesDC;
        int m_nBytesStatus;
        int m_nCyclesGS;

        // 74HC595 connections
        pi_gpio::PinOutput m_out_x_sin;
        pi_gpio::PinOutput m_out_x_sclk;
        pi_gpio::PinOutput m_out_x_latch;
        pi_gpio::PinOutput m_out_x_enable;
        // TLC5940 connections
        pi_gpio::PinOutput m_out_y_sin;
        pi_gpio::PinOutput m_out_y_latch;
        pi_gpio::PinOutput m_out_y_blank;
        pi_gpio::PinOutput m_out_y_gsclk;
        pi_gpio::PinOutput m_out_y_vprg;
        pi_gpio::PinOutput m_out_y_dcprg;
        pi_gpio::PinInput m_in_sout;
        pi_gpio::PinInput m_in_error;

        // GS buffers
        buffer_type m_bufferGS0;
        buffer_type m_bufferGS1;
        buffer_type m_bufferGS2;
        buffer_type* m_pbufferGSBack;
        const buffer_type* m_pbufferGSMid;
        const buffer_type* m_pbufferGSFront;
        std::atomic<bool> m_bGSAvailable;

        // DC buffer
        buffer_type m_bufferDC;
        std::atomic<bool> m_bDC;
        bool m_bFirstCycle;

        // Status buffer
        buffer_type m_bufferStatus;
        std::atomic<bool> m_bPushStatus;
        std::atomic<bool> m_bPopStatus;

        // XERR state
        bool m_bError;
    };

} // namespace quetzal

#endif // QUETZAL_DRIVERCONTROLLER_HPP
