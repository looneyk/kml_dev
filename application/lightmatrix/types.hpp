#if !defined(QUETZAL_LIGHTMATRIX_TYPES_HPP)
#define QUETZAL_LIGHTMATRIX_TYPES_HPP
//------------------------------------------------------------------------------
// Light Matrix Simulation
// types.hpp
//------------------------------------------------------------------------------

#include "quetzal/geometry/Point.hpp"
#include "quetzal/math/CartesianTraits.hpp"

namespace quetzal::lightmatrix
{

    using point_type = geometry::Point<math::CartesianTraits<float, 2>>;
    using vector_type = point_type::vector_type;

} // namespace quetzal::lightmatrix

#endif // QUETZAL_LIGHTMATRIX_TYPES_HPP
