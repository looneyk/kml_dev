//------------------------------------------------------------------------------
// stratum_sim
// ZoneEntryIndex.cpp
//------------------------------------------------------------------------------

#include "ZoneEntryIndex.hpp"
#include <unordered_map>

//------------------------------------------------------------------------------
quetzal::stratum_sim::ZoneEntryIndex::ZoneEntryIndex(size_t zone_count_max) :
    m_zone_entry_index(),
    m_zone_count_max(zone_count_max)
{
}

//------------------------------------------------------------------------------
std::vector<flecs::entity> quetzal::stratum_sim::ZoneEntryIndex::select_unloads(const std::vector<flecs::entity>& zones,
    flecs::entity zone_entered)
{
    // <zone_id, zone>
    std::unordered_map<PersistentId, flecs::entity> zones_by_id;

    for (flecs::entity zone : zones)
    {
        zones_by_id.emplace(zone.get<PersistentId>(), zone);
    }

    PersistentId zone_entered_id = zone_entered.get<PersistentId>();

    // Drops the earlier entry of the zone being entered, and the entries of zones no longer in the world, however they were
    // unloaded
    std::erase_if(m_zone_entry_index, [&](const auto& entry)
    {
        return entry.second == zone_entered_id || !zones_by_id.contains(entry.second);
    });

    m_zone_entry_index.emplace(++m_entry_count, zone_entered_id);

    std::vector<flecs::entity> unloads;
    size_t zone_count = zones.size();

    for (const auto& [entry_count, zone_id] : m_zone_entry_index)
    {
        if (zone_count <= m_zone_count_max || zone_id == zone_entered_id)
        {
            break;
        }

        unloads.push_back(zones_by_id.at(zone_id));
        --zone_count;
    }

    return unloads;
}

//------------------------------------------------------------------------------
size_t quetzal::stratum_sim::ZoneEntryIndex::zone_count_max() const
{
    return m_zone_count_max;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::ZoneEntryIndex::set_zone_count_max(size_t zone_count_max)
{
    m_zone_count_max = zone_count_max;
    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::ZoneEntryIndex::clear()
{
    m_zone_entry_index.clear();
    m_entry_count = 0;
    return;
}
