//------------------------------------------------------------------------------
// quetzal common
// Config.cpp
//------------------------------------------------------------------------------

#include "Config.hpp"

using namespace std::literals;

//------------------------------------------------------------------------------
std::string quetzal::Config::operator[](const std::string& key) const
{
    if (!m_values.contains(key))
    {
        return ""s;
    }

    return const_cast<map_type&>(m_values)[key];
}

//------------------------------------------------------------------------------
void quetzal::Config::insert(const std::string& key, const std::string& value)
{
    m_values[key] = value;
}
