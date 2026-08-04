#if !defined(QUETZAL_GEOMETRY_POLYGONMAPPING_HPP)
#define QUETZAL_GEOMETRY_POLYGONMAPPING_HPP
//------------------------------------------------------------------------------
// geometry
// PolygonMapping.hpp
//------------------------------------------------------------------------------

#include "Attributes.hpp"
#include "Point.hpp"
#include "Projection.hpp"
#include "quetzal/math/floating_point.hpp"
#include <array>
#include <cmath>
#include <span>
#include <cassert>

namespace quetzal::geometry
{

    //--------------------------------------------------------------------------
    // Maps a position within a polygon to the corresponding texcoord value given its vertices as Attribute values
    // Three vertices are required for an affine map, four for bilinear interpolation
    // Vertices must all be non-colinear
    // Bilinear is used rather than projective to be compatibe with lerp (as used elsewhere) on edges
    // Textured constrains the constructor rather than the class so that the type can still be named
    // where A has no texcoord, which is what an optional member or local in such a context requires
    template<typename A>
    class PolygonMapping
    {
    public:

        using attributes_type = A;
        using traits_type = A::traits_type;
        using value_type = A::value_type;
        using point_type = A::point_type;
        using vector_type = A::vector_type;
        using texcoord_type = A::texcoord_type;
        using projection_type = Projection<traits_type>;
        using projected_type = projection_type::projected_type;
        using projected_vector_type = projected_type::vector_type;

        PolygonMapping() = delete;
        PolygonMapping(std::span<const A> vertices, const vector_type& normal) requires quetzal::geometry::Textured<A>; // normal used for projection
        PolygonMapping(const PolygonMapping&) = default;
        PolygonMapping(PolygonMapping&&) = default;
        ~PolygonMapping() = default;

        PolygonMapping& operator=(const PolygonMapping&) = default;
        PolygonMapping& operator=(PolygonMapping&&) = default;

        size_t vertex_count() const;

        texcoord_type operator()(const point_type& position) const;

    private:

        texcoord_type affine(const projected_type& p) const;
        texcoord_type bilinear(const projected_type& p) const;

        std::array<projected_type, 4> m_positions; // Projected positions
        std::array<texcoord_type, 4> m_texcoords;
        size_t m_vertex_count = 0;
        projection_type m_projection;
    };

} // namespace quetzal::geometry

//------------------------------------------------------------------------------
template<typename A>
quetzal::geometry::PolygonMapping<A>::PolygonMapping(std::span<const A> vertices, const vector_type& normal) requires quetzal::geometry::Textured<A> :
    m_positions(),
    m_texcoords(),
    m_vertex_count(vertices.size() > 3 ? 4 : vertices.size()),
    m_projection(normal)
{
    assert(m_vertex_count >= 3);

    for (size_t i = 0; i < m_vertex_count; ++i)
    {
        assert(!colinear(vertices[(i + m_vertex_count - 1) % m_vertex_count].position(), vertices[i].position(), vertices[(i + 1) % m_vertex_count].position()));

        m_positions[i] = m_projection(vertices[i].position());
        m_texcoords[i] = vertices[i].texcoord();
    }
}

//------------------------------------------------------------------------------
template<typename A>
size_t quetzal::geometry::PolygonMapping<A>::vertex_count() const
{
    return m_vertex_count;
}

//------------------------------------------------------------------------------
template<typename A>
typename quetzal::geometry::PolygonMapping<A>::texcoord_type quetzal::geometry::PolygonMapping<A>::operator()(const point_type& position) const
{
    projected_type p = m_projection(position);
    return m_vertex_count == 3 ? affine(p) : bilinear(p);
}

//------------------------------------------------------------------------------
// Barycentric blend of the three vertex texcoords. Equivalent to affine_mapping, kept explicit here
// so that the mapping does not depend on the transformation matrix machinery
//------------------------------------------------------------------------------
template<typename A>
typename quetzal::geometry::PolygonMapping<A>::texcoord_type quetzal::geometry::PolygonMapping<A>::affine(const projected_type& p) const
{
    projected_vector_type v0 = m_positions[1] - m_positions[0];
    projected_vector_type v1 = m_positions[2] - m_positions[0];
    projected_vector_type v2 = p - m_positions[0];

    value_type d = dot_perp(v0, v1);
    assert(!math::float_eq0(d, v0.norm() * v1.norm()));

    value_type b1 = dot_perp(v2, v1) / d;
    value_type b2 = dot_perp(v0, v2) / d;

    // b0 is implicit: t0 + b1(t1 - t0) + b2(t2 - t0) is the barycentric blend with b0 = 1 - b1 - b2
    return m_texcoords[0] + (m_texcoords[1] - m_texcoords[0]) * b1 + (m_texcoords[2] - m_texcoords[0]) * b2;
}

//------------------------------------------------------------------------------
// Invert the position patch to recover (u, v), then evaluate the texcoord patch there.
//
// With a = P0, b = P1 - P0, c = P3 - P0, d = P0 - P1 + P2 - P3, the patch is a + bu + cv + duv, and d
// is the twist term that vanishes for a parallelogram. Writing q = p - a and eliminating u gives a
// quadratic in v; when the twist vanishes that quadratic degenerates to linear, which is the affine
// case and is handled separately rather than dividing by nearly zero
//------------------------------------------------------------------------------
template<typename A>
typename quetzal::geometry::PolygonMapping<A>::texcoord_type quetzal::geometry::PolygonMapping<A>::bilinear(const projected_type& p) const
{
    projected_vector_type b = m_positions[1] - m_positions[0];
    projected_vector_type c = m_positions[3] - m_positions[0];
    projected_vector_type d = (m_positions[0] - m_positions[1]) + (m_positions[2] - m_positions[3]);
    projected_vector_type q = p - m_positions[0];

    value_type a2 = dot_perp(c, d);
    value_type a1 = dot_perp(c, b) - dot_perp(q, d);
    value_type a0 = -dot_perp(q, b);

    value_type v = value_type(0);

    // a2 is a difference of two products, so a twist that is mathematically zero survives only as
    // cancellation noise proportional to them, and has to be judged against their scale. A regular
    // polygon of more than four sides makes c and d exactly antiparallel and lands here every time
    if (math::float_eq0(a2, c.norm() * d.norm()))
    {
        assert(!math::float_eq0(a1));
        v = -a0 / a1;
    }
    else
    {
        value_type discriminant = a1 * a1 - value_type(4) * a2 * a0;
        assert(discriminant >= value_type(0));
        value_type root = std::sqrt(discriminant);

        // Adding the term that shares a1's sign avoids the cancellation the direct form suffers on
        // whichever root is the smaller. Which of the two that produces depends on the sign of a1, so
        // they are ordered back the way the direct form gives them, leaving the preference below
        // deciding between the same two as before rather than silently swapping them
        value_type q2 = -(a1 + (a1 < value_type(0) ? -root : root)) / value_type(2);
        value_type va = q2 / a2;
        value_type vb = math::float_eq0(q2) ? va : a0 / q2;

        value_type v0 = a1 < value_type(0) ? vb : va;
        value_type v1 = a1 < value_type(0) ? va : vb;

        // Two roots, one of which lies on the patch. Prefer the one within the unit interval
        v = (v0 >= value_type(0) && v0 <= value_type(1)) ? v0 : v1;
    }

    // u from u(b + dv) = q - cv, using whichever component of the denominator is better conditioned
    projected_vector_type denominator = b + d * v;
    projected_vector_type numerator = q - c * v;
    value_type u = std::abs(denominator.x()) >= std::abs(denominator.y()) ? numerator.x() / denominator.x() : numerator.y() / denominator.y();

    return lerp(lerp(m_texcoords[0], m_texcoords[1], u), lerp(m_texcoords[3], m_texcoords[2], u), v);
}

#endif // QUETZAL_GEOMETRY_POLYGONMAPPING_HPP
