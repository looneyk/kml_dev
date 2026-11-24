#if !defined(QUETZAL_STRATUM_CORE_REGIONEXTENT_HPP)
#define QUETZAL_STRATUM_CORE_REGIONEXTENT_HPP
//------------------------------------------------------------------------------
// stratum_core
// RegionExtent.hpp
//------------------------------------------------------------------------------

#include "types.hpp"
#include "quetzal/math/Interval.hpp"
#include <optional>
#include <vector>

namespace quetzal::stratum_core
{

    //--------------------------------------------------------------------------
    // The set of zones that make up a region: a union of rectangular worldspace zones or a list of zone positions
    struct RegionExtent
    {
        enum class Arrangement
        {
            Boxes,
            Positions
        };

        // Each axis is an inclusive range, and an unspecified axis interval indicates that it spans the parent's extent on that axis
        struct Box
        {
            std::optional<math::Interval<coord_type>> x_extent;
            std::optional<math::Interval<coord_type>> y_extent;
            std::optional<math::Interval<coord_type>> z_extent;
        };

        Arrangement arrangement = Arrangement::Boxes;
        std::vector<Box> boxes;
        std::vector<discrete_point3_type> positions;
    };

} // namespace quetzal::stratum_core

#endif // QUETZAL_STRATUM_CORE_REGIONEXTENT_HPP
