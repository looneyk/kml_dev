#if !defined(QUETZAL_STRATUM_SIM_ZONEENTRYINDEX_HPP)
#define QUETZAL_STRATUM_SIM_ZONEENTRYINDEX_HPP
//------------------------------------------------------------------------------
// stratum_sim
// ZoneEntryIndex.hpp
//------------------------------------------------------------------------------

#include "types.hpp"
#include <flecs.h>
#include <cstdint>
#include <map>
#include <vector>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // Chooses the zones to unload: those entered longest ago, once there are more than a maximum
    class ZoneEntryIndex
    {
    public:

        explicit ZoneEntryIndex(size_t zone_count_max);

        // Marks zone_entered as the most recently entered zone, then chooses which of the world's zones to unload. Never
        // chooses zone_entered or a zone that was never entered
        std::vector<flecs::entity> select_unloads(const std::vector<flecs::entity>& zones, flecs::entity zone_entered);

        size_t zone_count_max() const;
        void set_zone_count_max(size_t zone_count_max);

        // Forgets every entry, keeping the maximum
        void clear();

    private:

        // <entry_count, zone_id>
        std::map<std::uint64_t, PersistentId> m_zone_entry_index;

        std::uint64_t m_entry_count = 0;
        size_t m_zone_count_max;
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_ZONEENTRYINDEX_HPP
