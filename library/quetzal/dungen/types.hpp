#if !defined(QUETZAL_DUNGEN_TYPES_HPP)
#define QUETZAL_DUNGEN_TYPES_HPP
//------------------------------------------------------------------------------
// dungen
// types.hpp
//------------------------------------------------------------------------------

#include "quetzal/common/id.hpp"
#include "quetzal/math/DiscreteVector.hpp"
#include "quetzal/geometry/DiscreteBox.hpp"
#include "quetzal/geometry/DiscretePoint.hpp"
#include "quetzal/math/Random.hpp"

namespace quetzal::dungen
{

    using coord_type = long;
    using vector_type = math::DiscreteVector<coord_type, 2>;
    using point_type = geometry::DiscretePoint<coord_type, 2>;
    using box_type = geometry::DiscreteBox<coord_type, 2>;
    using random_type = math::Random<>;

} // namespace quetzal::dungen

#endif // QUETZAL_DUNGEN_TYPES_HPP
