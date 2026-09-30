//------------------------------------------------------------------------------
// stratum_sim
// SavedGameDetails.cpp
//------------------------------------------------------------------------------

#include "SavedGameDetails.hpp"

//------------------------------------------------------------------------------
quetzal::stratum_sim::SavedGameDetails::SavedGameDetails(std::string_view name, std::string_view description, const time_type& time) :
    m_name(name),
    m_description(description),
    m_time(time)
{
}
