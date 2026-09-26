#if !defined(QUETZAL_DUNGEN_TYPES_HPP)
#define QUETZAL_DUNGEN_TYPES_HPP
//------------------------------------------------------------------------------
// dungen
// types.hpp
//------------------------------------------------------------------------------

#include "quetzal/common/id.hpp"
#include "quetzal/math/DiscreteVector.hpp"
#include "quetzal/geometry/DiscreteBox.hpp"
#include "quetzal/geometry/DiscreteDirectedPoint.hpp"
#include "quetzal/geometry/DiscretePoint.hpp"
#include "quetzal/math/Random.hpp"
#include <functional>
#include <string>
#include <cstdint>

namespace quetzal::dungen
{

    using coord_type = std::int32_t;
    using point_type = geometry::DiscretePoint<coord_type, 2>;
    using vector_type = point_type::vector_type;
    using box_type = geometry::DiscreteBox<coord_type, 2>;
    using directed_point_type = geometry::DiscreteDirectedPoint<coord_type, 2>;
    using random_type = math::Random<>;

    // Called with what the generator did short of what was asked, where that is not an error
    using warning_function_type = std::function<void(const std::string& warning)>;

    using math::CardinalDirection;

    inline const auto& cardinal_directions = math::cardinal_directions<coord_type, 2>;

    inline const vector_type& cardinal_direction(CardinalDirection direction)
    {
        return math::cardinal_direction<coord_type, 2>(direction);
    }

} // namespace quetzal::dungen

#endif // QUETZAL_DUNGEN_TYPES_HPP
