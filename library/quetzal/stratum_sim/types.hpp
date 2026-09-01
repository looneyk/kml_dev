#if !defined(QUETZAL_STRATUM_SIM_TYPES_HPP)
#define QUETZAL_STRATUM_SIM_TYPES_HPP
//------------------------------------------------------------------------------
// stratum_sim
// types.hpp
//------------------------------------------------------------------------------

#include "quetzal/geometry/DirectedPoint.hpp"
#include "quetzal/geometry/Point.hpp"
#include "quetzal/geometry/Polygon.hpp"
#include "quetzal/math/CartesianTraits.hpp"
#include "quetzal/math/DiscreteVector.hpp"
#include <vector>

namespace quetzal::stratum_sim
{

    using value_type = float;
    using traits_type = math::CartesianTraits<value_type, 3>;
    using point_type = geometry::Point<traits_type>;
    using vector_type = point_type::vector_type;
    using points_type = std::vector<point_type>;
    using polygon_type = geometry::Polygon<traits_type>;

    // Where a mover is and which way it faces. Relative movement passes its own direction;
    // absolute movement passes one the view chose and leaves the mover's alone
    using placement_type = geometry::DirectedPoint<traits_type>;

    //--------------------------------------------------------------------------
    // Displacement on the grid: a step between cells, an extent in cells, an offset from
    // one cell to another. What a GridPosition is not
    using DiscreteVector = math::DiscreteVector<long, 2>;

    //--------------------------------------------------------------------------
    // The four steps a flat grid can take, named. A holder of constants rather than an
    // enumeration, so that a direction is the displacement itself and nothing has to map
    // one to the other
    struct Direction
    {
        inline static const DiscreteVector East = DiscreteVector(1, 0);
        inline static const DiscreteVector North = DiscreteVector(0, 1);
        inline static const DiscreteVector West = DiscreteVector(-1, 0);
        inline static const DiscreteVector South = DiscreteVector(0, -1);
    };

    //--------------------------------------------------------------------------
    // The horizontal plane the grid lies in. A layout is flat, so anything asked of the grid
    // comes to it through here rather than by dropping a component somewhere further down
    using footprint_traits = traits_type::resized_traits<2>;
    using footprint_vector_type = math::Vector<footprint_traits>;

    inline footprint_vector_type footprint(const vector_type& v)
    {
        return footprint_vector_type(v.x(), v.y());
    }

    // The cell step a direction through space most nearly asks for, height disregarded
    inline DiscreteVector grid_step(const vector_type& direction)
    {
        return math::direction<DiscreteVector>(footprint(direction));
    }

    //--------------------------------------------------------------------------
    // Which cell of the grid, as a coordinate rather than a stored record. The grid it sits
    // on is recorded at load rather than derived from the wall geometry afterwards
    // A position rather than a displacement, so it is only ever moved by adding a
    // DiscreteVector to it
    struct GridPosition
    {
        long i = 0;
        long j = 0;

        GridPosition& operator+=(const DiscreteVector& v);

        friend bool operator==(const GridPosition&, const GridPosition&) = default;
    };

    //--------------------------------------------------------------------------
    inline GridPosition& GridPosition::operator+=(const DiscreteVector& v)
    {
        i += v.x();
        j += v.y();
        return *this;
    }

    //--------------------------------------------------------------------------
    inline GridPosition operator+(GridPosition position, const DiscreteVector& v)
    {
        return position += v;
    }

    //--------------------------------------------------------------------------
    inline DiscreteVector operator-(const GridPosition& a, const GridPosition& b)
    {
        return DiscreteVector(a.i - b.i, a.j - b.j);
    }

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_TYPES_HPP
