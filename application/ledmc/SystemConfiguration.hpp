#if !defined(QUETZAL_SYSTEMCONFIGURATION_HPP)
#define QUETZAL_SYSTEMCONFIGURATION_HPP
//------------------------------------------------------------------------------
// SystemConfiguration.hpp
//------------------------------------------------------------------------------

#include <string>
#include <iosfwd>

namespace quetzal
{

    //--------------------------------------------------------------------------
    class SystemConfiguration
    {
    public:

        SystemConfiguration();
        SystemConfiguration(const SystemConfiguration&) = default;
        ~SystemConfiguration() = default;

        SystemConfiguration& operator=(const SystemConfiguration&) = default;

        int channel_bits_gs() const;
        int channel_bits_dc() const;
        int device_bits_status() const;
        int device_channels() const;
        int strip_devices() const;
        int strips() const;

        // GPIO numbers for 74HC595 connections
        int gpio_ser() const;
        int gpio_srclk() const;
        int gpio_rclk() const;
        int gpio_oe() const;
        // GPIO numbers for TLC5940 connections
        int gpio_sin() const;
        int gpio_xlat() const;
        int gpio_blank() const;
        int gpio_gsclk() const;
        int gpio_vprg() const;
        int gpio_dcprg() const;
        int gpio_sout() const;
        int gpio_xerr() const;

        std::string dot_correction_filename() const;

        bool load(const std::string& filename); // Load configuration from file
        bool store(const std::string& filename) const; // Store configuration from file

    private:

        int m_nChannelBitsGS;
        int m_nChannelBitsDC;
        int m_nDeviceBitsStatus;
        int m_nDeviceChannels;
        int m_nStripDevices;
        int m_nStrips;

        // GPIO
        int m_gpio_ser;
        int m_gpio_srclk;
        int m_gpio_rclk;
        int m_gpio_oe;
        int m_gpio_sin;
        int m_gpio_xlat;
        int m_gpio_blank;
        int m_gpio_gsclk;
        int m_gpio_vprg;
        int m_gpio_dcprg;
        int m_gpio_sout;
        int m_gpio_xerr;

        std::string m_dot_correction_filename;

        friend std::ostream& operator<<(std::ostream& os, const SystemConfiguration& sc);
    };

    std::ostream& operator<<(std::ostream& os, const SystemConfiguration& sc);

} // namespace quetzal

#endif // QUETZAL_SYSTEMCONFIGURATION_HPP
