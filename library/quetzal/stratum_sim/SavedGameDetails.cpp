//------------------------------------------------------------------------------
// stratum_sim
// SavedGameDetails.cpp
//------------------------------------------------------------------------------

#include "SavedGameDetails.hpp"

//------------------------------------------------------------------------------
quetzal::stratum_sim::SavedGameDetails::SavedGameDetails(std::string_view save_name, std::string_view character_name, std::string_view description,
    const time_type& time) :
    m_save_name(save_name),
    m_character_name(character_name),
    m_description(description),
    m_time(time)
{
}

//------------------------------------------------------------------------------
const std::string& quetzal::stratum_sim::SavedGameDetails::save_name() const
{
    return m_save_name;
}

//------------------------------------------------------------------------------
const std::string& quetzal::stratum_sim::SavedGameDetails::character_name() const
{
    return m_character_name;
}

//------------------------------------------------------------------------------
const std::string& quetzal::stratum_sim::SavedGameDetails::description() const
{
    return m_description;
}

//------------------------------------------------------------------------------
const quetzal::stratum_sim::SavedGameDetails::time_type& quetzal::stratum_sim::SavedGameDetails::time() const
{
    return m_time;
}
