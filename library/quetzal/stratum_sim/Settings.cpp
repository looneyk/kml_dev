//------------------------------------------------------------------------------
// stratum_sim
// Settings.cpp
//------------------------------------------------------------------------------

#include "Settings.hpp"

//------------------------------------------------------------------------------
quetzal::stratum_sim::Settings::Settings() :
    m_random_seeding_on_open(RandomSeedingOnOpen::RestoreSavedState)
{
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::RandomSeedingOnOpen quetzal::stratum_sim::Settings::random_seeding_on_open() const
{
    return m_random_seeding_on_open;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::Settings::set_random_seeding_on_open(RandomSeedingOnOpen random_seeding_on_open)
{
    m_random_seeding_on_open = random_seeding_on_open;
    return;
}
