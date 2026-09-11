#if !defined(QUETZAL_STRATUM_VIEW_TYPES_HPP)
#define QUETZAL_STRATUM_VIEW_TYPES_HPP
//------------------------------------------------------------------------------
// stratum_view
// types.hpp
//------------------------------------------------------------------------------

#include "quetzal/geometry/DiscretePoint.hpp"
#include "quetzal/math/DiscreteVector.hpp"
#include <cstdint>

namespace quetzal::stratum_view
{

    using coord_type = std::int32_t;
    using vector_type = math::DiscreteVector<coord_type, 2>;
    using point_type = geometry::DiscretePoint<coord_type, 2>;

} // namespace quetzal::stratum_view

#endif // QUETZAL_STRATUM_VIEW_TYPES_HPP
