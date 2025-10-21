#if !defined(QUETZAL_GEOMETRY_VECTOR_HPP)
#define QUETZAL_GEOMETRY_VECTOR_HPP
//------------------------------------------------------------------------------
// geometry
// Vector.hpp
//------------------------------------------------------------------------------

#include "quetzal/math/Vector.hpp"

namespace quetzal::geometry
{

    template<typename Traits>
    using Vector = math::Vector<Traits>;

} // namespace quetzal::geometry

#endif // QUETZAL_GEOMETRY_VECTOR_HPP
