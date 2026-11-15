#if !defined(QUETZAL_STRATUM_SIM_PROXIMITY_INDEX_BASE_HPP)
#define QUETZAL_STRATUM_SIM_PROXIMITY_INDEX_BASE_HPP
//------------------------------------------------------------------------------
// stratum_sim
// ProximityIndexBase.hpp
//------------------------------------------------------------------------------

#include "types.hpp"
#include "quetzal/common/id.hpp"
#include <vector>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // The entities placed in one zone, by location. Each layout type supplies an implementation
    class ProximityIndexBase
    {
    public:

        using entity_ids_type = std::vector<id_type>;

        virtual ~ProximityIndexBase() = default;

        // Adds the entity at the placement, or moves it there if it is already in the index
        virtual void place(id_type entity_id, const placement_type& placement) = 0;

        // The entity must be in the index
        virtual void remove(id_type entity_id) = 0;

        virtual bool contains(id_type entity_id) const = 0;
        virtual size_t entity_count() const = 0;

        // The entities at the same location as the placement, in ascending id order
        virtual entity_ids_type entities_at(const placement_type& placement) const = 0;

        // The entities whose positions are within the range of the placement's position, in ascending id order
        virtual entity_ids_type entities_within(const placement_type& placement, value_type range) const = 0;

    protected:

        ProximityIndexBase() = default;
        ProximityIndexBase(const ProximityIndexBase&) = default;
        ProximityIndexBase(ProximityIndexBase&&) noexcept = default;

        ProximityIndexBase& operator=(const ProximityIndexBase&) = default;
        ProximityIndexBase& operator=(ProximityIndexBase&&) noexcept = default;
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_PROXIMITY_INDEX_BASE_HPP
