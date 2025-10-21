#if !defined(QUETZAL_MODEL_PATTERN_PANE_HPP)
#define QUETZAL_MODEL_PATTERN_PANE_HPP
//------------------------------------------------------------------------------
// model_pattern
// Pane.hpp
//------------------------------------------------------------------------------

#include "quetzal/common/id.hpp"
#include "quetzal/geometry/AxisAlignedBoundingBox.hpp"
#include "quetzal/geometry/Point.hpp"
#include "quetzal/geometry/Polygon.hpp"
#include "quetzal/geometry/transformation_matrix.hpp"
#include "quetzal/math/HomogeneousTraits.hpp"
#include <vector>
#include <cassert>

namespace quetzal
{

    //--------------------------------------------------------------------------
    class Pane
    {
    public:

        using traits_type = math::HomogeneousTraits<double, 2>;
        using point_type = geometry::Point<traits_type>;
        using vector_type = point_type::vector_type;
        using polygon_type = geometry::Polygon<traits_type>;
        using bounds_type = geometry::AxisAlignedBoundingBox<traits_type>;
        using references_type = std::vector<id_type>;

        Pane() = default;
        Pane(id_type id, const polygon_type& polygon, const vector_type& direction, size_t nSection = 1, size_t nSections = 1);
        Pane(const Pane&) = default;
        Pane(Pane&&) = default;
        ~Pane() = default;

        Pane& operator=(const Pane&) = default;
        Pane& operator=(Pane&&) = default;

        const polygon_type& polygon() const;
        const bounds_type& bounds() const;
        const vector_type& direction() const; // Grain direction using texture y-axis mapped to vertex coordinate system
        const vector_type size() const;
        double area() const; // rectangular area of bounds, assumes that all vertices are in a plane parallel to the xy-plane

        size_t section_count() const;
        size_t section() const;

        size_t reference_count() const;
        const references_type& references() const;
        void insert_reference(id_type id);

    private:

        polygon_type m_polygon;
        vector_type m_direction;
        bounds_type m_bounds;
        references_type m_references;
        size_t m_nSection;
        size_t m_nSections;
    };

    geometry::AxisAlignedBoundingBox<Pane::traits_type> pane_bounds(const Pane::polygon_type& polygon);

    bool float_eq(const Pane& lhs, const Pane& rhs);

} // namespace quetzal

#endif // QUETZAL_MODEL_PATTERN_PANE_HPP
