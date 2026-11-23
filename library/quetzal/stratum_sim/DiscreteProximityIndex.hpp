#if !defined(QUETZAL_STRATUM_SIM_DISCRETE_PROXIMITY_INDEX_HPP)
#define QUETZAL_STRATUM_SIM_DISCRETE_PROXIMITY_INDEX_HPP
//------------------------------------------------------------------------------
// stratum_sim
// DiscreteProximityIndex.hpp
//------------------------------------------------------------------------------

#include "ProximityIndexBase.hpp"
#include "types.hpp"
#include "quetzal/common/id.hpp"
#include <unordered_map>

namespace quetzal::stratum_sim
{

    class DiscreteLayout;

    //--------------------------------------------------------------------------
    // The entities placed in one zone of a discrete layout, by the cell each is in
    class DiscreteProximityIndex final : public ProximityIndexBase
    {
    public:

        // The layout must outlive the index
        explicit DiscreteProximityIndex(const DiscreteLayout& discrete_layout);

        void place(id_type entity_id, const placement_type& placement) override;
        void remove(id_type entity_id) override;

        bool contains(id_type entity_id) const override;
        size_t entity_count() const override;

        // The entities in the placement's cell
        entity_ids_type entities_at(const placement_type& placement) const override;
        entity_ids_type entities_at(const discrete_point_type& cell_position) const;

        // Visits the cells within a given distance from placement position, or every entity if there are fewer entities than cells
        entity_ids_type entities_within(const placement_type& placement, value_type distance) const override;

    private:

        size_t slot_at(const placement_type& placement) const;
        void insert_entity_id(size_t slot, id_type entity_id);
        void erase_entity_id(size_t slot, id_type entity_id);

        const DiscreteLayout& m_discrete_layout;
        std::unordered_map<size_t, entity_ids_type> m_entity_ids_by_slot; // <slot, entity ids>
        std::unordered_map<id_type, placement_type> m_placements_by_entity_id; // <entity id, placement>
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_DISCRETE_PROXIMITY_INDEX_HPP
