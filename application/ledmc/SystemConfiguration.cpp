//------------------------------------------------------------------------------
// SystemConfiguration.cpp
//------------------------------------------------------------------------------

#include "SystemConfiguration.hpp"
#include "quetzal/xml/xml.hpp"
#include "quetzal/Exception.hpp"
#include "quetzal/string_util.hpp"
#include <fstream>
#include <sstream>
#include <ostream>

using namespace std;

namespace
{

    const string s_system_configuration = "system_configuration";
    const string s_channel_bits_gs = "channel_bits_gs";
    const string s_channel_bits_dc = "channel_bits_dc";
    const string s_device_bits_status = "device_bits_status";
    const string s_device_channels = "device_channels";
    const string s_strip_devices = "strip_devices";
    const string s_strips = "strips";
    const string s_gpio_ser = "gpio_ser";
    const string s_gpio_srclk = "gpio_srclk";
    const string s_gpio_rclk = "gpio_rclk";
    const string s_gpio_oe = "gpio_oe";
    const string s_gpio_sin = "gpio_sin";
    const string s_gpio_xlat = "gpio_xlat";
    const string s_gpio_blank = "gpio_blank";
    const string s_gpio_gsclk = "gpio_gsclk";
    const string s_gpio_vprg = "gpio_vprg";
    const string s_gpio_dcprg = "gpio_dcprg";
    const string s_gpio_sout = "gpio_sout";
    const string s_gpio_xerr = "gpio_xerr";
    const string s_dot_correction_filename = "dot_correction_filename";

} // namespace

//------------------------------------------------------------------------------
quetzal::SystemConfiguration::SystemConfiguration() :
    m_nChannelBitsGS(12),
    m_nChannelBitsDC(6),
    m_nDeviceBitsStatus(192),
    m_nDeviceChannels(16),
    m_nStripDevices(3),
    m_nStrips(16),
    m_gpio_ser(17),
    m_gpio_srclk(27),
    m_gpio_rclk(22),
    m_gpio_oe(23),
    m_gpio_sin(24),
    m_gpio_xlat(5),
    m_gpio_blank(6),
    m_gpio_gsclk(13),
    m_gpio_vprg(19),
    m_gpio_dcprg(26),
    m_gpio_sout(0),
    m_gpio_xerr(0),
    m_dot_correction_filename("dot_correction.xml")
{
}

//------------------------------------------------------------------------------
int quetzal::SystemConfiguration::channel_bits_gs() const
{
    return m_nChannelBitsGS;
}

//------------------------------------------------------------------------------
int quetzal::SystemConfiguration::channel_bits_dc() const
{
    return m_nChannelBitsDC;
}

//------------------------------------------------------------------------------
int quetzal::SystemConfiguration::device_bits_status() const
{
    return m_nDeviceBitsStatus;
}

//------------------------------------------------------------------------------
int quetzal::SystemConfiguration::device_channels() const
{
    return m_nDeviceChannels;
}

//------------------------------------------------------------------------------
int quetzal::SystemConfiguration::strip_devices() const
{
    return m_nStripDevices;
}

//------------------------------------------------------------------------------
int quetzal::SystemConfiguration::strips() const
{
    return m_nStrips;
}

//------------------------------------------------------------------------------
int quetzal::SystemConfiguration::gpio_ser() const
{
    return m_gpio_ser;
}

//------------------------------------------------------------------------------
int quetzal::SystemConfiguration::gpio_srclk() const
{
    return m_gpio_srclk;
}

//------------------------------------------------------------------------------
int quetzal::SystemConfiguration::gpio_rclk() const
{
    return m_gpio_rclk;
}

//------------------------------------------------------------------------------
int quetzal::SystemConfiguration::gpio_oe() const
{
    return m_gpio_oe;
}

//------------------------------------------------------------------------------
int quetzal::SystemConfiguration::gpio_sin() const
{
    return m_gpio_sin;
}

//------------------------------------------------------------------------------
int quetzal::SystemConfiguration::gpio_xlat() const
{
    return m_gpio_xlat;
}

//------------------------------------------------------------------------------
int quetzal::SystemConfiguration::gpio_blank() const
{
    return m_gpio_blank;
}

//------------------------------------------------------------------------------
int quetzal::SystemConfiguration::gpio_gsclk() const
{
    return m_gpio_gsclk;
}

//------------------------------------------------------------------------------
int quetzal::SystemConfiguration::gpio_vprg() const
{
    return m_gpio_vprg;
}

//------------------------------------------------------------------------------
int quetzal::SystemConfiguration::gpio_dcprg() const
{
    return m_gpio_dcprg;
}

//------------------------------------------------------------------------------
int quetzal::SystemConfiguration::gpio_sout() const
{
    return m_gpio_sout;
}

//------------------------------------------------------------------------------
int quetzal::SystemConfiguration::gpio_xerr() const
{
    return m_gpio_xerr;
}

//------------------------------------------------------------------------------
string quetzal::SystemConfiguration::dot_correction_filename() const
{
    return m_dot_correction_filename;
}

//------------------------------------------------------------------------------
bool quetzal::SystemConfiguration::load(const string& filename)
{
    xml::Document document;

    ifstream ifs(filename);
    if (!ifs.is_open())
    {
        ostringstream oss;
        oss << "Error opening xml file: " << filename << ": ";
        throw Exception(oss.str(), __FILE__, __LINE__);
    }

    ifs >> document;

    const xml::Element& emain = document.element();
    if (emain.name() != s_system_configuration)
    {
        ostringstream oss;
        oss << "Error parsing xml file " << filename << ": ";
        oss << "Not a valid system configuration file";
        throw Exception(oss.str(), __FILE__, __LINE__);
    }

    // Load elements
    for (auto& pe : emain.elements())
    {
        string name = pe->name();
        if (name == s_channel_bits_gs)
        {
            m_nChannelBitsGS = to_type<int>(pe->value());
        }
        else if (name == s_channel_bits_dc)
        {
            m_nChannelBitsDC = to_type<int>(pe->value());
        }
        else if (name == s_device_bits_status)
        {
            m_nDeviceBitsStatus = to_type<int>(pe->value());
        }
        else if (name == s_device_channels)
        {
            m_nDeviceChannels = to_type<int>(pe->value());
        }
        else if (name == s_strip_devices)
        {
            m_nStripDevices = to_type<int>(pe->value());
        }
        else if (name == s_strips)
        {
            m_nStrips = to_type<int>(pe->value());
        }
        else if (name == s_gpio_ser)
        {
            m_gpio_ser = to_type<int>(pe->value());
        }
        else if (name == s_gpio_srclk)
        {
            m_gpio_srclk = to_type<int>(pe->value());
        }
        else if (name == s_gpio_rclk)
        {
            m_gpio_rclk = to_type<int>(pe->value());
        }
        else if (name == s_gpio_oe)
        {
            m_gpio_oe = to_type<int>(pe->value());
        }
        else if (name == s_gpio_sin)
        {
            m_gpio_sin = to_type<int>(pe->value());
        }
        else if (name == s_gpio_xlat)
        {
            m_gpio_xlat = to_type<int>(pe->value());
        }
        else if (name == s_gpio_blank)
        {
            m_gpio_blank = to_type<int>(pe->value());
        }
        else if (name == s_gpio_gsclk)
        {
            m_gpio_gsclk = to_type<int>(pe->value());
        }
        else if (name == s_gpio_vprg)
        {
            m_gpio_vprg = to_type<int>(pe->value());
        }
        else if (name == s_gpio_dcprg)
        {
            m_gpio_dcprg = to_type<int>(pe->value());
        }
        else if (name == s_gpio_sout)
        {
            m_gpio_sout = to_type<int>(pe->value());
        }
        else if (name == s_gpio_xerr)
        {
            m_gpio_xerr = to_type<int>(pe->value());
        }
        else if (name == s_dot_correction_filename)
        {
            m_dot_correction_filename = pe->value();
        }
    }

    return true;
}

//------------------------------------------------------------------------------
bool quetzal::SystemConfiguration::store(const string& filename) const
{
    xml::Document document;
    xml::Element& emain = document.element();
    emain.set_name(s_system_configuration);

    emain.add_element(s_channel_bits_gs, to_string(m_nChannelBitsGS));
    emain.add_element(s_channel_bits_dc, to_string(m_nChannelBitsDC));
    emain.add_element(s_device_bits_status, to_string(m_nDeviceBitsStatus));
    emain.add_element(s_device_channels, to_string(m_nDeviceChannels));
    emain.add_element(s_strip_devices, to_string(m_nStripDevices));
    emain.add_element(s_strips, to_string(m_nStrips));
    emain.add_element(s_gpio_ser, to_string(m_gpio_ser));
    emain.add_element(s_gpio_srclk, to_string(m_gpio_srclk));
    emain.add_element(s_gpio_rclk, to_string(m_gpio_rclk));
    emain.add_element(s_gpio_oe, to_string(m_gpio_oe));
    emain.add_element(s_gpio_sin, to_string(m_gpio_sin));
    emain.add_element(s_gpio_xlat, to_string(m_gpio_xlat));
    emain.add_element(s_gpio_blank, to_string(m_gpio_blank));
    emain.add_element(s_gpio_gsclk, to_string(m_gpio_gsclk));
    emain.add_element(s_gpio_vprg, to_string(m_gpio_vprg));
    emain.add_element(s_gpio_dcprg, to_string(m_gpio_dcprg));
    emain.add_element(s_gpio_sout, to_string(m_gpio_sout));
    emain.add_element(s_gpio_xerr, to_string(m_gpio_xerr));
    emain.add_element(s_dot_correction_filename, m_dot_correction_filename);

    ofstream ofs(filename);
    if (ofs.fail())
    {
        return false;
    }

    ofs << document;

    return true;
}

//------------------------------------------------------------------------------
ostream& quetzal::operator<<(ostream& os, const SystemConfiguration& sc)
{
    os << s_channel_bits_gs << "          " << sc.m_nChannelBitsGS << endl;
    os << s_channel_bits_dc << "          " << sc.m_nChannelBitsDC << endl;
    os << s_device_bits_status << "       " << sc.m_nDeviceBitsStatus << endl;
    os << s_device_channels << "          " << sc.m_nDeviceChannels << endl;
    os << s_strip_devices << "            " << sc.m_nStripDevices << endl;
    os << s_strips << "                   " << sc.m_nStrips << endl;
    os << s_gpio_ser << "                 " << sc.m_gpio_ser << endl;
    os << s_gpio_srclk << "               " << sc.m_gpio_srclk << endl;
    os << s_gpio_rclk << "                " << sc.m_gpio_rclk << endl;
    os << s_gpio_oe << "                  " << sc.m_gpio_oe << endl;
    os << s_gpio_sin << "                 " << sc.m_gpio_sin << endl;
    os << s_gpio_xlat << "                " << sc.m_gpio_xlat << endl;
    os << s_gpio_blank << "               " << sc.m_gpio_blank << endl;
    os << s_gpio_gsclk << "               " << sc.m_gpio_gsclk << endl;
    os << s_gpio_vprg << "                " << sc.m_gpio_vprg << endl;
    os << s_gpio_dcprg << "               " << sc.m_gpio_dcprg << endl;
    os << s_gpio_sout << "                " << sc.m_gpio_sout << endl;
    os << s_gpio_xerr << "                " << sc.m_gpio_xerr << endl;
    os << s_dot_correction_filename << "  " << sc.m_dot_correction_filename << endl;

    return os;
}
