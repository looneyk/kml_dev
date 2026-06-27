//------------------------------------------------------------------------------
// DateTime.cpp
//------------------------------------------------------------------------------

#include "DateTime.hpp"
#include <format>
#include <iostream>
#include <sstream>

using namespace std;

//------------------------------------------------------------------------------
quetzal::DateTime::DateTime() :
    m_tp(chrono::system_clock::now())
{
}

//------------------------------------------------------------------------------
string quetzal::DateTime::to_string(string_view spec) const
{
    string s = spec.empty() ? "%F %T" : string(spec);
    chrono::zoned_time zt{chrono::current_zone(), m_tp};
    return vformat("{:" + s + "}", make_format_args(zt));
}

//------------------------------------------------------------------------------
bool quetzal::DateTime::from_string(string_view str, string_view spec)
{
    istringstream stream{ string(str) };

    string s = spec.empty() ? "%F %T" : string(spec);

    chrono::local_seconds lt;
    if (stream >> chrono::parse(s, lt))
    {
        try
        {
            m_tp = chrono::current_zone()->to_sys(lt);
            return true;
        }
        catch (const exception&)
        {
            return false;
        }
    }

    return false;
}

//------------------------------------------------------------------------------
string quetzal::DateTime::to_string_utc(string_view spec) const
{
    string s = spec.empty() ? "%F %T" : string(spec);
    return vformat("{:" + s + "}", make_format_args(m_tp));
}

//------------------------------------------------------------------------------
bool quetzal::DateTime::from_string_utc(string_view str, string_view spec)
{
    istringstream stream{ string(str) };

    string s = spec.empty() ? "%F %T" : string(spec);

    chrono::sys_seconds tp;
    if (stream >> chrono::parse(s, tp))
    {
        m_tp = tp;
        return true;
    }

    return false;
}

//------------------------------------------------------------------------------
int quetzal::DateTime::days_until(const DateTime& dt) const
{
    return chrono::duration_cast<chrono::hours>(dt.m_tp - m_tp).count() / 24;
}

//------------------------------------------------------------------------------
ostream& quetzal::operator<<(ostream& os, const DateTime& dt)
{
    os << dt.to_string();
    return os;
}

//------------------------------------------------------------------------------
istream& quetzal::operator>>(istream& is, DateTime& dt)
{
    string date_str;
    string time_str;
    if (is >> date_str >> time_str)
    {
        if (!dt.from_string(date_str + " " + time_str))
        {
            is.setstate(ios::failbit);
        }
    }

    return is;
}
