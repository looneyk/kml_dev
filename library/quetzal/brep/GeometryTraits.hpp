#if !defined(QUETZAL_BREP_GEOMETRYTRAITS_HPP)
#define QUETZAL_BREP_GEOMETRYTRAITS_HPP
//------------------------------------------------------------------------------
// brep
// GeometryTraits.hpp
//------------------------------------------------------------------------------

#include "quetzal/geometry/Attributes.hpp"
#include "quetzal/geometry/HalfSpace.hpp"
#include "quetzal/geometry/Line.hpp"
#include "quetzal/geometry/Plane.hpp"
#include "quetzal/geometry/Point.hpp"
#include "quetzal/geometry/Polygon.hpp"
#include "quetzal/geometry/Projection.hpp"
#include "quetzal/geometry/Ray.hpp"
#include "quetzal/geometry/Segment.hpp"
#include "quetzal/math/Matrix.hpp"
#include "quetzal/math/MatrixTraits.hpp"
#include "quetzal/math/Vector.hpp"

namespace quetzal::brep
{

    //--------------------------------------------------------------------------
    template<typename Traits, typename MT = math::MatrixTraits<typename Traits::value_type, Traits::size>>
    class GeometryTraits
    {
    public:

        using matrix_type = math::Matrix<MT>;
        using vector_type = math::Vector<Traits>;
        using point_type = geometry::Point<Traits>;
        using line_type = geometry::Line<Traits>;
        using ray_type = geometry::Ray<Traits>;
        using segment_type = geometry::Segment<Traits>;
        using plane_type = geometry::Plane<Traits>;
        using polygon_type = geometry::Polygon<Traits>;
        using halfspace_type = geometry::HalfSpace<Traits>;
        using projection_type = geometry::Projection<Traits>;
        using attributes_type = geometry::Attributes<Traits>;

        static constexpr size_t dimension = Traits::dimension;
    };

} // namespace quetzal::brep

#endif // QUETZAL_BREP_GEOMETRYTRAITS_HPP
