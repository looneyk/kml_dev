#if !defined(QUETZAL_GEOMETRY_POLYGON_HPP)
#define QUETZAL_GEOMETRY_POLYGON_HPP
//------------------------------------------------------------------------------
// geometry
// Polygon.hpp
//------------------------------------------------------------------------------

#include "AxisAlignedBoundingBox.hpp"
#include "Orientation.hpp"
#include "Plane.hpp"
#include "PointRelation.hpp"
#include "Point.hpp"
#include "PositionAccessor.hpp"
#include "Projection.hpp"
#include "Segment.hpp"
#include "spatial_properties.hpp"
#include "transformation_matrix.hpp"
#include "triangle_util.hpp"
#include "quetzal/common/string_util.hpp"
#include "quetzal/math/Interval.hpp"
#include "quetzal/math/floating_point.hpp"
#include <algorithm>
#include <cctype>
#include <charconv>
#include <exception>
#include <format>
#include <istream>
#include <iterator>
#include <optional>
#include <ostream>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>
#include <cassert>

namespace quetzal::geometry
{

    // Polygon is an ordered set of coplanar vertices
    // Each sequential pair of vertices defines an edge, the last vertex wrapping around to the first
    // edge count == vertex count
    // Note that planarity is assumed but not often enforced
    // Results of centroid, area, normal, and plane are undefined if the polygon is non-planar or degenerate
    // Sections: nullopt - smooth, empty - edge-based, otherwise sequential section start edge ids
    // (ids for polygon vertices are just array positions)

    //--------------------------------------------------------------------------
    template<typename Traits>
    class Polygon : public PointRelation<Traits>
    {
    public:

        using traits_type = Traits;
        using size_type = Traits::size_type;
        using value_type = Traits::value_type;
        using point_type = Point<Traits>;
        using vector_type = point_type::vector_type;
        using vertex_type = point_type;
        using vertices_type = std::vector<vertex_type>;
        using edge_type = Segment<Traits>;
        using matrix_type = transformation_matrix_type<Traits>;

        Polygon() = default;
        explicit Polygon(size_type n);
        explicit Polygon(std::span<point_type> points);
        Polygon(std::initializer_list<vertex_type> points);
        Polygon(const Polygon&) = default;
        Polygon(Polygon&&) = default;
        virtual ~Polygon() = default;

        template<typename InputIterator>
        Polygon(InputIterator first, InputIterator last);

        Polygon& operator=(const Polygon&) = default;
        Polygon& operator=(Polygon&&) = default;

        Polygon clone() const; // Creates a copy that can be modified, non-final

        const vertices_type& vertices() const;
        vertices_type& vertices();

        size_t vertex_count() const noexcept;
        const vertex_type& vertex(size_type n) const;
        vertex_type& vertex(size_type n);
        void set_vertex(size_type n, const point_type& point);

        size_t edge_count() const noexcept;
        edge_type edge(size_type n) const;

        bool planar() const;

        point_type centroid() const;
        point_type vertex_centroid() const; // The geometric mean of all vertices

        // instead of passing in normal for areas, just use internal normal ...

        value_type area() const requires (Traits::dimension == 2);
        value_type area(const vector_type& normal) const requires (Traits::dimension == 3);

        // Based on expressed or implied CCW winding order
        value_type signed_area() const requires (Traits::dimension == 2);
        value_type signed_area(const vector_type& normal) const requires (Traits::dimension == 3);

        // Assumes CCW winding order
        vector_type normal() const requires (Traits::dimension == 3);
        Plane<Traits> plane() const requires (Traits::dimension == 3);

        void transform(const matrix_type& matrix);
        void scale(value_type factor);

        // Returns less, equivalent, greater for interior, boundary, exterior respectively
        // Exterior is defined by the winding order
        std::partial_ordering compare(const point_type& point, int tolerance = math::tolerance_default) const override;

        virtual void clear();

        AxisAlignedBoundingBox<Traits> bounds() const;

        // Sections allow for distinct sets of contiguous vertices
        // sections nullopt indicates a smooth polygon
        // sections empty indicates that each edge is a section
        // otherwise sections indicates the starting edge number of each section sequentially starting with section 0
        // Note that single (no sections) and sections with a single section of 0 are equivalent, but use separate code paths
        using sections_type = std::optional<std::vector<size_type>>;

        const sections_type& sections() const;
        sections_type& sections();

        // Single smooth section
        bool sections_single() const;
        void set_sections_single();

        // Edge based sections
        bool sections_edges() const;
        void set_sections_edges();

        // Fully specified sections
        bool sections_full() const;
        void set_sections_full(); // Initializes to a single section starting at edge 0

        // Orientation, lazy evaluation
        vector_type right() const;
        Orientation<Traits> orientation() const requires (Traits::dimension == 3); // direction = normal

        // Indicate that the set of vertices is fixed and no further changes will be made, so precalculated values can be provided or default orientation can be used
        // This has the effect of precluding the calculation of orientation aspects entirely, so only call this if you don't want their automatic calculation
        void set_final(const vector_type& right = vector_type::x_direction()) requires (Traits::dimension == 2);
        void set_final(const vector_type& normal = vector_type::z_direction(), const vector_type& right = vector_type::x_direction()) requires (Traits::dimension == 3);
        // set_final_normal, set_final_right to allow for the other to be calculated? ...
        // 3D right optional? ...
        // reset_final? ...

        // Point is in the region defined by the polygon
        // Result is undefined if the point is on an edge
        // These should be internal, but need to be public ...
        bool contains_(const point_type& point, int tolerance = math::tolerance_default) const requires (Traits::dimension == 2);
        bool contains_(const point_type& point, int tolerance = math::tolerance_default) const requires (Traits::dimension == 3 && quetzal::math::Homogeneous<Traits>);
        bool contains_(const point_type& point, int tolerance = math::tolerance_default) const requires (Traits::dimension == 3 && quetzal::math::Cartesian<Traits>);

    private:

        void clear_cached();

        vertices_type m_vertices;
        sections_type m_sections;

        mutable std::optional<vector_type> m_normal;
        mutable std::optional<vector_type> m_right;
        mutable std::optional<AxisAlignedBoundingBox<Traits>> m_bounds;
        bool m_final = false;
    };

    template<typename Traits>
    Polygon<Traits> transform(Polygon<Traits> polygon, const typename Polygon<Traits>::matrix_type& matrix);

    template<typename Traits>
    Polygon<Traits> scale(Polygon<Traits> polygon, typename Traits::value_type factor);

    // Acts on vertices leaving everything else the same
    template<typename Traits>
    Polygon<Traits> lerp(const Polygon<Traits>& a, const Polygon<Traits>& b, typename Polygon<Traits>::value_type t);

    template<typename Traits>
    Polygon<Traits> invert(const Polygon<Traits>& polygon);

    // Regular polygon
    // Position generation is not symmetric
    template<typename Traits> requires (Traits::dimension == 2)
    Polygon<Traits> regular_polygon(size_t n, typename Traits::value_type radius);

    // Regular polygon in the xy-plane
    // Position generation is symmetric
    template<typename Traits> requires (Traits::dimension == 3)
    Polygon<Traits> regular_polygon(size_t n, typename Traits::value_type radius, typename Traits::value_type z = Traits::value_type(0));

    // Section of a regular polygon in the xy-plane
    // Position generation is symmetric
    // The last edge is the connection in the case of non-unit azimuth interval
    // In the case of a unit azimuth interval, this edge will have 0 length
    // n is the number of reguilar polygon edges, not counting the connecting edge, so the resulting polygon section will have n + 1 vertices
    template<typename Traits> requires (Traits::dimension == 3)
    Polygon<Traits> regular_polygon_section(size_t n, typename Traits::value_type radius, typename Traits::value_type z = Traits::value_type(0), math::Interval<typename Traits::value_type> azimuth_interval = math::strict_unit_interval());

    template<typename Traits>
    std::ostream& operator<<(std::ostream& os, const Polygon<Traits>& polygon);

    // Reads vertices while a series delimiter follows the last one read
    template<typename Traits>
    std::istream& operator>>(std::istream& is, Polygon<Traits>& polygon);

    // Reads vertices while a series delimiter follows the last one read
    template<typename Traits>
    std::from_chars_result from_chars(const char* first, const char* last, Polygon<Traits>& polygon, const FormatSpec& spec = {});

    template<typename Traits>
    std::to_chars_result to_chars(char* first, char* last, const Polygon<Traits>& polygon, const FormatSpec& spec = {});

    //--------------------------------------------------------------------------
    template<typename Traits>
    struct PositionAccessor<Polygon<Traits>>
    {
        using point_type = Polygon<Traits>::point_type;
        using vector_type = point_type::vector_type;
        using value_type = point_type::value_type;
        static constexpr size_t dimension = point_type::dimension;

        static size_t position_count(const Polygon<Traits>& t)
        {
            return t.vertex_count();
        }

        static point_type position(const Polygon<Traits>& t, size_t n)
        {
            if (n >= t.vertex_count())
            {
                throw std::out_of_range("PositionAccessor::range_check");
            }

            return t.vertex(n);
        }
    
        static bool constant_time_acccess()
        {
            return true;
        }
    };

} // namespace quetzal::geometry

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Polygon<Traits>::Polygon(size_type n) :
    PointRelation<Traits>(),
    m_vertices(n),
    m_sections(),
    m_normal(),
    m_right(),
    m_bounds(),
    m_final(false)
{
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Polygon<Traits>::Polygon(std::span<point_type> points) :
    PointRelation<Traits>(),
    m_vertices(points.begin(), points.end()),
    m_sections(),
    m_normal(),
    m_right(),
    m_bounds(),
    m_final(false)
{
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Polygon<Traits>::Polygon(std::initializer_list<vertex_type> points) :
    PointRelation<Traits>(),
    m_vertices(points),
    m_sections(),
    m_normal(),
    m_right(),
    m_bounds(),
    m_final(false)
{
}

//------------------------------------------------------------------------------
template<typename Traits>
template<typename InputIterator>
quetzal::geometry::Polygon<Traits>::Polygon(InputIterator first, InputIterator last) :
    PointRelation<Traits>(),
    m_vertices(),
    m_sections(),
    m_normal(),
    m_right(),
    m_bounds(),
    m_final(false)
{
    std::copy(first, last, std::back_inserter(m_vertices));
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::Polygon<Traits> quetzal::geometry::Polygon<Traits>::clone() const
{
    Polygon<Traits> result = *this;
    result.m_final = false;
    return result;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::Polygon<Traits>::vertices_type& quetzal::geometry::Polygon<Traits>::vertices()
{
    clear_cached();
    return m_vertices;
}

//------------------------------------------------------------------------------
template<typename Traits>
const typename quetzal::geometry::Polygon<Traits>::vertices_type& quetzal::geometry::Polygon<Traits>::vertices() const
{
    return m_vertices;
}

//------------------------------------------------------------------------------
template<typename Traits>
size_t quetzal::geometry::Polygon<Traits>::vertex_count() const noexcept
{
    return m_vertices.size();
}

//------------------------------------------------------------------------------
template<typename Traits>
const typename quetzal::geometry::Polygon<Traits>::vertex_type& quetzal::geometry::Polygon<Traits>::vertex(size_type n) const
{
    assert(n < m_vertices.size());
    return m_vertices[n];
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::Polygon<Traits>::vertex_type& quetzal::geometry::Polygon<Traits>::vertex(size_type n)
{
    assert(n < m_vertices.size());
    clear_cached();
    return m_vertices[n];
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::Polygon<Traits>::set_vertex(size_type n, const point_type& point)
{
    assert(n < m_vertices.size());
    clear_cached();
    m_vertices[n] = point;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
size_t quetzal::geometry::Polygon<Traits>::edge_count() const noexcept
{
    return m_vertices.size();
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::Polygon<Traits>::edge_type quetzal::geometry::Polygon<Traits>::edge(size_type n) const
{
    assert(n < m_vertices.size());
    return {m_vertices[n], m_vertices[(n + 1) % m_vertices.size()]};
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::Polygon<Traits>::planar() const
{
    return geometry::planar(*this);
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::Polygon<Traits>::point_type quetzal::geometry::Polygon<Traits>::centroid() const
{
    return geometry::centroid(*this);
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::Polygon<Traits>::point_type quetzal::geometry::Polygon<Traits>::vertex_centroid() const
{
    return geometry::vertex_centroid(*this);
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::Polygon<Traits>::value_type quetzal::geometry::Polygon<Traits>::area() const requires (Traits::dimension == 2)
{
    return geometry::area(*this);
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::Polygon<Traits>::value_type quetzal::geometry::Polygon<Traits>::area(const vector_type& normal) const requires (Traits::dimension == 3)
{
    return geometry::area(*this, normal);
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::Polygon<Traits>::value_type quetzal::geometry::Polygon<Traits>::signed_area() const requires (Traits::dimension == 2)
{
    return geometry::signed_area(*this);
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::Polygon<Traits>::value_type quetzal::geometry::Polygon<Traits>::signed_area(const vector_type& normal) const requires (Traits::dimension == 3)
{
    return geometry::signed_area(*this, normal);
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::Polygon<Traits>::vector_type quetzal::geometry::Polygon<Traits>::normal() const requires (Traits::dimension == 3)
{
    if (!m_normal)
    {
        m_normal = geometry::normal(*this);
    }

    return *m_normal;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Plane<Traits> quetzal::geometry::Polygon<Traits>::plane() const requires (Traits::dimension == 3)
{
    return Plane<Traits>(m_vertices[0], normal());
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::Polygon<Traits>::transform(const matrix_type& matrix)
{
    clear_cached();

    for (auto& vertex : m_vertices)
    {
        vertex *= matrix;
    }

    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::Polygon<Traits>::scale(value_type factor)
{
    clear_cached();

    for (auto& vertex : m_vertices)
    {
        vertex *= factor;
    }

    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
std::partial_ordering quetzal::geometry::Polygon<Traits>::compare(const point_type& point, int tolerance) const
{
    for (size_t i = 0; i < edge_count(); ++i)
    {
        auto segment = edge(i);
        if (segment.contains(point, tolerance))
        {
            return std::partial_ordering::equivalent;
        }
    }

    return contains_(point, tolerance) ? std::partial_ordering::less : std::partial_ordering::greater;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::Polygon<Traits>::clear()
{
    clear_cached();
    m_vertices.clear();
    m_sections.reset();
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::AxisAlignedBoundingBox<Traits> quetzal::geometry::Polygon<Traits>::bounds() const
{
    if (!m_bounds)
    {
        m_bounds = geometry::bounds(*this);
    }

    return *m_bounds;
}

//------------------------------------------------------------------------------
template<typename Traits>
const typename quetzal::geometry::Polygon<Traits>::sections_type& quetzal::geometry::Polygon<Traits>::sections() const
{
    return m_sections;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::Polygon<Traits>::sections_type& quetzal::geometry::Polygon<Traits>::sections()
{
    return m_sections;
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::Polygon<Traits>::sections_single() const
{
    return !m_sections;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::Polygon<Traits>::set_sections_single()
{
    m_sections.reset();
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::Polygon<Traits>::sections_edges() const
{
    return m_sections && m_sections.value().empty();
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::Polygon<Traits>::set_sections_edges()
{
    m_sections.emplace();
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::Polygon<Traits>::sections_full() const
{
    return m_sections && !m_sections.value().empty();
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::Polygon<Traits>::set_sections_full()
{
    m_sections.emplace(0);
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::Polygon<Traits>::vector_type quetzal::geometry::Polygon<Traits>::right() const
{
    if (!m_right)
    {
        m_right = normalize(m_vertices[0] - geometry::centroid(*this));
    }

    return *m_right;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Orientation<Traits> quetzal::geometry::Polygon<Traits>::orientation() const requires (Traits::dimension == 3)
{
    return {normal(), right()};
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::Polygon<Traits>::set_final(const vector_type& right) requires (Traits::dimension == 2)
{
    m_right = right;
    m_final = true;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::Polygon<Traits>::set_final(const vector_type& normal, const vector_type& right) requires (Traits::dimension == 3)
{
    m_normal = normal;
    m_right = right;
    m_final = true;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::Polygon<Traits>::contains_(const point_type& point, int tolerance) const requires (Traits::dimension == 2)
{
    // Return >0: left, ==0: on, <0: right for p2 relative to the line through p0 and p1
    auto left = [tolerance](point_type p0, point_type p1, point_type p2)
    {
        auto t = (p1.x() - p0.x()) * (p2.y() - p0.y()) - (p2.x() -  p0.x()) * (p1.y() - p0.y());
        return math::float_compare0(t, tolerance);
    };

    auto vertices = m_vertices;
    vertices.push_back(m_vertices[0]);

    size_t n = 0; // the  winding number counter

    for (size_t i = 0; i < edge_count(); ++i)
    {
        if (math::float_le(vertices[i].y(), point.y(), tolerance))
        {
            if (math::float_gt(vertices[i + 1].y(), point.y(), tolerance)) // upward crossing
                 if (left(vertices[i], vertices[i + 1], point) > 0)
                     ++n; // valid up intersect
        }
        else
        {
            if (math::float_le(vertices[i + 1].y(), point.y(), tolerance)) // downward crossing
                 if (left(vertices[i], vertices[i + 1], point) < 0)
                     --n; // valid down intersect
        }
    }

    return n != 0;
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::Polygon<Traits>::contains_(const point_type& point, int tolerance) const requires (Traits::dimension == 3 && quetzal::math::Homogeneous<Traits>)
{
    if (!planar() || !plane().contains(point, tolerance))
    {
        return false;
    }

    using projection_type = Projection<Traits>;
    using projected_traits = projection_type::projected_traits;
    using projected_type = projection_type::projected_type;

    projection_type projection(normal());
    Polygon<projected_traits> polygon_projected;
    std::transform(m_vertices.begin(), m_vertices.end(), std::back_inserter(polygon_projected.vertices()), [&](const point_type& point) -> projected_type { return projection(point); });
    return polygon_projected.contains_(projection(point), tolerance);
}

//------------------------------------------------------------------------------
// Cartesian traits do not support Projection, so the axis the polygon most nearly faces is dropped. That can reverse
// handedness, which a nonzero winding test does not notice
template<typename Traits>
bool quetzal::geometry::Polygon<Traits>::contains_(const point_type& point, int tolerance) const requires (Traits::dimension == 3 && quetzal::math::Cartesian<Traits>)
{
    if (!planar() || !plane().contains(point, tolerance))
    {
        return false;
    }

    using projected_traits = typename Traits::template resized_traits<2>;
    using projected_type = Point<projected_traits>;

    vector_type v = normal();
    size_t axis = 0;
    value_type extent = v[0] * v[0];

    if (v[1] * v[1] > extent)
    {
        axis = 1;
        extent = v[1] * v[1];
    }

    if (v[2] * v[2] > extent)
    {
        axis = 2;
    }

    size_t i0 = (axis + 1) % 3;
    size_t i1 = (axis + 2) % 3;

    auto drop = [i0, i1](const point_type& p) -> projected_type { return projected_type(p[i0], p[i1]); };

    Polygon<projected_traits> polygon_projected;
    std::transform(m_vertices.begin(), m_vertices.end(), std::back_inserter(polygon_projected.vertices()), drop);
    return polygon_projected.contains_(drop(point), tolerance);
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::Polygon<Traits>::clear_cached()
{
    assert(!m_final);
    m_normal.reset();
    m_right.reset();
    m_bounds.reset();
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Polygon<Traits> quetzal::geometry::transform(Polygon<Traits> polygon, const typename Polygon<Traits>::matrix_type& matrix)
{
    auto result = polygon.clone();
    result.transform(matrix);
    return result;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Polygon<Traits> quetzal::geometry::scale(Polygon<Traits> polygon, typename Traits::value_type factor)
{
    auto result = polygon.clone();
    result.scale(factor);
    return result;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Polygon<Traits> quetzal::geometry::lerp(const Polygon<Traits>& a, const Polygon<Traits>& b, typename Polygon<Traits>::value_type t)
{
    assert(a.vertex_count() == b.vertex_count());
    assert(a.sections() == b.sections());

    size_t n = a.vertex_count();
    Polygon<Traits> polygon(n);
    polygon.sections() = a.sections();

    for (size_t i = 0; i < n; ++i)
    {
        polygon.vertex(i) = lerp(a.vertex(i), b.vertex(i), t);
    }

    return polygon;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Polygon<Traits> quetzal::geometry::invert(const Polygon<Traits>& polygon)
{
    auto reversed = polygon.vertices() | std::views::reverse;
    return {reversed.begin(), reversed.end()};
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension == 2)
quetzal::geometry::Polygon<Traits> quetzal::geometry::regular_polygon(size_t n, typename Traits::value_type radius)
{
// this could also be done symmetrically ...
    Polygon<Traits> polygon(n);
    for (size_t i = 0; i < n; ++i)
    {
        auto phi = math::PiTwo<typename Polygon<Traits>::value_type> * i / n;
        auto& position = polygon.vertex(i);
        position.set_x(radius * math::denoise011(cos(phi)));
        position.set_y(radius * math::denoise011(sin(phi)));
    }

    return polygon;
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension == 3)
quetzal::geometry::Polygon<Traits> quetzal::geometry::regular_polygon(size_t n, typename Traits::value_type radius, typename Traits::value_type z)
{
    using T = Traits::value_type;

    Polygon<Traits> polygon(n);
    auto& vertices = polygon.vertices();

    // this from vertices_attributes, fix for n rather than n + 1 positions ...

    size_t n2 = n / 2;

    if (math::even(n)) // Use first quadrant positions for reference
    {
        size_t n4 = n2 / 2;

        if (math::even(n2)) // Use first octant positions for reference
        {
            size_t n8 = n4 / 2;

            size_t i0 = 0;
            size_t i1 = n8;
            for (size_t i = i0; i <= i1; ++i)
            {
                T t = T(i) / T(n);
                T phi = math::PiTwo<T> * t;
                T cos_phi = math::denoise011(cos(phi));
                T sin_phi = math::denoise011(sin(phi));
                vertices[i] = {radius * cos_phi, radius * sin_phi, z};

                size_t j = n4 - i;
                if (j != i)
                {
                    vertices[j] = {vertices[i].y(), vertices[i].x(), z};
                }
            }
        }
        else
        {
            size_t i0 = 0;
            size_t i1 = n4;
            for (size_t i = i0; i <= i1; ++i)
            {
                T t = T(i) / T(n);
                T phi = math::PiTwo<T> * t;
                T cos_phi = math::denoise011(cos(phi));
                T sin_phi = math::denoise011(sin(phi));
                vertices[i] = {radius * cos_phi, radius * sin_phi, z};
            }
        }

        size_t i0 = n4 + 1;
        size_t i1 = n2;
        for (size_t i = i0; i <= i1; ++i)
        {
            size_t j = n2 - i;
            vertices[i] = {-vertices[j].x(), vertices[j].y(), z};
        }

        i0 = n2 + 1;
        i1 = n;
//        for (size_t i = i0; i <= i1; ++i)
        for (size_t i = i0; i < i1; ++i)
        {
            size_t j = n - i;
if (j < n)
            vertices[i] = {vertices[j].x(), -vertices[j].y(), z};
        }
    }
    else // Use first and second quadrant positions for reference
    {
        for (size_t i = 0; i <= n2; ++i)
        {
            T t = T(i) / T(n);
            T phi = math::PiTwo<T> * t;
            T cos_phi = math::denoise011(cos(phi));
            T sin_phi = math::denoise011(sin(phi));
            vertices[i] = {radius * cos_phi, radius * sin_phi, z};

            size_t j = n - i;
if (j < n)
            vertices[j] = {vertices[i].x(), -vertices[i].y(), z};
        }
    }

    return polygon;
}

//------------------------------------------------------------------------------
template<typename Traits> requires (Traits::dimension == 3)
quetzal::geometry::Polygon<Traits> quetzal::geometry::regular_polygon_section(size_t n, typename Traits::value_type radius, typename Traits::value_type z, math::Interval<typename Traits::value_type> azimuth_interval)
{
    using T = Traits::value_type;

    Polygon<Traits> polygon(n + 1);
    auto& vertices = polygon.vertices();

    if (azimuth_interval.strict_unit())
    {
        size_t n2 = n / 2;

        if (math::even(n)) // Use first quadrant positions for reference
        {
            size_t n4 = n2 / 2;

            if (math::even(n2)) // Use first octant positions for reference
            {
                size_t n8 = n4 / 2;

                size_t i0 = 0;
                size_t i1 = n8;
                for (size_t i = i0; i <= i1; ++i)
                {
                    T t = T(i) / T(n);
                    T phi = math::PiTwo<T> * azimuth_interval.lerp(t);
                    T cos_phi = math::denoise011(cos(phi));
                    T sin_phi = math::denoise011(sin(phi));
                    vertices[i] = {radius * cos_phi, radius * sin_phi, z};

                    size_t j = n4 - i;
                    if (j != i)
                    {
                        vertices[j] = {vertices[i].y(), vertices[i].x(), z};
                    }
                }
            }
            else
            {
                size_t i0 = 0;
                size_t i1 = n4;
                for (size_t i = i0; i <= i1; ++i)
                {
                    T t = T(i) / T(n);
                    T phi = math::PiTwo<T> * azimuth_interval.lerp(t);
                    T cos_phi = math::denoise011(cos(phi));
                    T sin_phi = math::denoise011(sin(phi));
                    vertices[i] = {radius * cos_phi, radius * sin_phi, z};
                }
            }

            size_t i0 = n4 + 1;
            size_t i1 = n2;
            for (size_t i = i0; i <= i1; ++i)
            {
//                T t = T(i) / T(n);
                size_t j = n2 - i;
                vertices[i] = {-vertices[j].x(), vertices[j].y(), z};
            }

            i0 = n2 + 1;
            i1 = n;
            for (size_t i = i0; i <= i1; ++i)
            {
//                T t = T(i) / T(n);
                size_t j = n - i;
                vertices[i] = {vertices[j].x(), -vertices[j].y(), z};
            }
        }
        else // Use first and second quadrant positions for reference
        {
            for (size_t i = 0; i <= n2; ++i)
            {
                T t = T(i) / T(n);
                T phi = math::PiTwo<T> * azimuth_interval.lerp(t);
                T cos_phi = math::denoise011(cos(phi));
                T sin_phi = math::denoise011(sin(phi));
                vertices[i] = {radius * cos_phi, radius * sin_phi, z};

                size_t j = n - i;
                vertices[j] = {vertices[i].x(), -vertices[i].y(), z};
            }
        }
    }
    else
    {
        for (size_t i = 0; i <= n; ++i)
        {
            T t = T(i) / T(n);
            T phi = math::PiTwo<T> * azimuth_interval.lerp(t);
            T cos_phi = math::denoise011(cos(phi));
            T sin_phi = math::denoise011(sin(phi));
            vertices[i] = {radius * cos_phi, radius * sin_phi, z};
        }

        if (azimuth_interval.unit())
        {
            vertices[n] = vertices[0];
        }
    }

    return polygon;
}

//------------------------------------------------------------------------------
template<typename Traits>
std::ostream& quetzal::geometry::operator<<(std::ostream& os, const Polygon<Traits>& polygon)
{
    os << quetzal::to_string(polygon, os);
    return os;
}

//------------------------------------------------------------------------------
template<typename Traits>
std::istream& quetzal::geometry::operator>>(std::istream& is, Polygon<Traits>& polygon)
{
    const std::string series_delimiter(quetzal::resolved_series_delimiter(quetzal::stream_format_spec(is), quetzal::series_delimiter_default));
    const std::string_view text = quetzal::trim_whitespace(series_delimiter);

    typename Polygon<Traits>::vertices_type vertices;
    while (true)
    {
        typename Polygon<Traits>::point_type vertex;
        if (!(is >> vertex))
        {
            return is;
        }

        vertices.push_back(vertex);

        if (is.eof())
        {
            break;
        }

        if (text.empty())
        {
            if ((is >> std::ws).eof())
            {
                break;
            }

            continue;
        }

        if (std::isspace(static_cast<unsigned char>(series_delimiter.front())))
        {
            is >> std::ws;
        }

        if (is.eof() || is.peek() != std::istream::traits_type::to_int_type(text.front()))
        {
            break;
        }

        quetzal::skip_pattern(is, series_delimiter);
    }

    polygon = Polygon<Traits>(std::span<typename Polygon<Traits>::point_type>(vertices));
    return is;
}

//------------------------------------------------------------------------------
template<typename Traits>
std::from_chars_result quetzal::geometry::from_chars(const char* first, const char* last, Polygon<Traits>& polygon, const FormatSpec& spec)
{
    const std::string_view series_delimiter = quetzal::resolved_series_delimiter(spec, quetzal::series_delimiter_default);

    typename Polygon<Traits>::vertices_type vertices;
    const char* position = first;
    while (true)
    {
        typename Polygon<Traits>::point_type vertex;
        auto result = from_chars(position, quetzal::find_pattern(position, last, series_delimiter), vertex, spec);
        if (result.ec != std::errc{})
        {
            return result;
        }

        vertices.push_back(vertex);

        const char* next = quetzal::skip_pattern(result.ptr, last, series_delimiter);
        if (next == nullptr || next == last)
        {
            polygon = Polygon<Traits>(std::span<typename Polygon<Traits>::point_type>(vertices));
            return result;
        }

        position = next;
    }
}

//------------------------------------------------------------------------------
template<typename Traits>
std::to_chars_result quetzal::geometry::to_chars(char* first, char* last, const Polygon<Traits>& polygon, const FormatSpec& spec)
{
    return quetzal::series_to_chars(first, last, polygon.vertices(), spec);
}

//------------------------------------------------------------------------------
template<typename Traits>
struct std::formatter<quetzal::geometry::Polygon<Traits>> : quetzal::series_formatter<quetzal::geometry::Point<Traits>>
{
    template<typename FormatContext>
    auto format(const quetzal::geometry::Polygon<Traits>& polygon, FormatContext& ctx) const
    {
        return this->format_series(polygon.vertices(), ctx);
    }
};

#endif // QUETZAL_GEOMETRY_POLYGON_HPP
