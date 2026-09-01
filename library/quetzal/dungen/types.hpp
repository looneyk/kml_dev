#if !defined(QUETZAL_DUNGEN_TYPES_HPP)
#define QUETZAL_DUNGEN_TYPES_HPP
//------------------------------------------------------------------------------
// dungen
// types.hpp
//------------------------------------------------------------------------------

#include "quetzal/common/id.hpp"
#include "quetzal/math/DiscreteVector.hpp"

namespace quetzal::dungen
{

    using coord_type = long;
    using vector_type = math::DiscreteVector<coord_type, 2>;
    using position_type = vector_type;
    using dimensions_type = vector_type;

} // namespace quetzal::dungen

#endif // QUETZAL_DUNGEN_TYPES_HPP
