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
#include <cstdint>

namespace quetzal::dungen
{

    using coord_type = std::int32_t;
    using vector_type = math::DiscreteVector<coord_type, 2>;
    using point_type = geometry::DiscretePoint<coord_type, 2>;
    using box_type = geometry::DiscreteBox<coord_type, 2>;
    using random_type = math::Random<>;

    using math::CardinalDirection;

    inline const auto& cardinal_directions = math::cardinal_directions<coord_type, 2>;

    inline const vector_type& cardinal_direction(CardinalDirection direction)
    {
        return math::cardinal_direction<coord_type, 2>(direction);
    }

} // namespace quetzal::dungen

#endif // QUETZAL_DUNGEN_TYPES_HPP
