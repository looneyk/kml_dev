#if !defined(QUETZAL_STRATUM_SIM_REGION_SPEC_NODE_HPP)
#define QUETZAL_STRATUM_SIM_REGION_SPEC_NODE_HPP
//------------------------------------------------------------------------------
// stratum_sim
// RegionSpecNode.hpp
//------------------------------------------------------------------------------

#include "RegionSpec.hpp"
#include "quetzal/common/id.hpp"
#include <vector>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // A node of a region spec tree: a region spec, with the ids of its parent and its subregions, each the slot of a spec in the tree
    struct RegionSpecNode
    {
        RegionSpec region_spec;
        id_type parent_id = nullid;
        std::vector<id_type> subregion_ids;
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_REGION_SPEC_NODE_HPP
