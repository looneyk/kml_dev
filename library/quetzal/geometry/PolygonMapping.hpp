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

    // Maps a position within a polygon to the texcoord that the polygon's corner attributes imply.
    //
    // Three corners give an affine map, which is exact. Four give a bilinear patch: linear along each
    // edge, with a uv cross term in the interior. That choice is made once at construction and kept
    // inside, so this is a plain value type -- copyable, storable, returnable -- with no polymorphism
    // to hold and no visitor needed to build one.
    //
    // Bilinear rather than projective is deliberate. Along an edge one parameter is constant and the
    // patch reduces exactly to a lerp of that edge's two corner values, which is what splitting an edge
    // does to a vertex's texcoord. The two therefore agree by construction. A projective map would
    // reparameterize along the edge and disagree with the split.
    //
    // Corners are taken in perimeter order, which is the (0,0) (1,0) (1,1) (0,1) assignment bilinear
    // wants. They must describe the polygon BEFORE it is subdivided: splitting an edge inserts a corner
    // collinear with its neighbours, and a fit through collinear corners is singular
    //--------------------------------------------------------------------------
    // Constraint is spelled with the fully qualified name so that it is token for token identical to
    // the one repeated on each out of class definition, which is what makes them match
    template<typename Attributes> requires (Attributes::contains(quetzal::geometry::AttributesFlags::Texcoord0))
    class PolygonMapping
    {
    public:

        using attributes_type = Attributes;
        using traits_type = Attributes::traits_type;
        using value_type = Attributes::value_type;
        using point_type = Attributes::point_type;
        using vector_type = Attributes::vector_type;
        using texcoord_type = Attributes::texcoord_type;
        using projection_type = Projection<traits_type>;
        using projected_type = projection_type::projected_type;
        using projected_vector_type = projected_type::vector_type;

        PolygonMapping() = default; // Not usable, tests false
        PolygonMapping(std::span<const Attributes> corners, const vector_type& normal);
        PolygonMapping(const PolygonMapping&) = default;
        PolygonMapping(PolygonMapping&&) = default;
        ~PolygonMapping() = default;

        PolygonMapping& operator=(const PolygonMapping&) = default;
        PolygonMapping& operator=(PolygonMapping&&) = default;

        // False where this polygon cannot define a mapping: too few corners, or corners too nearly
        // degenerate to fit. Whether the attribute type carries a texcoord at all is a separate,
        // compile time question, answered by Attributes::contains before naming this type
        explicit operator bool() const;

        size_t corner_count() const;

        texcoord_type operator()(const point_type& position) const;

    private:

        texcoord_type affine(const projected_type& p) const;
        texcoord_type bilinear(const projected_type& p) const;

        // z of the cross product of two vectors in the projection plane
        static value_type cross(const projected_vector_type& a, const projected_vector_type& b);

        static texcoord_type blend(const texcoord_type& a, const texcoord_type& b, value_type t);

        std::array<projected_type, 4> m_positions; // Projected once, up front
        std::array<texcoord_type, 4> m_texcoords;
        size_t m_corner_count = 0;
        projection_type m_projection;
    };

} // namespace quetzal::geometry

//------------------------------------------------------------------------------
template<typename Attributes> requires (Attributes::contains(quetzal::geometry::AttributesFlags::Texcoord0))
quetzal::geometry::PolygonMapping<Attributes>::PolygonMapping(std::span<const Attributes> corners, const vector_type& normal) :
    m_positions(),
    m_texcoords(),
    m_corner_count(corners.size() > 3 ? 4 : corners.size()),
    m_projection(normal)
{
    if (m_corner_count < 3)
    {
        m_corner_count = 0; // Not enough to define anything
        return;
    }

    for (size_t i = 0; i < m_corner_count; ++i)
    {
        m_positions[i] = m_projection(corners[i].position());
        m_texcoords[i] = corners[i].texcoord();
    }

    // A fit through collinear corners is singular, which is what an already subdivided polygon looks
    // like. Reject it here rather than returning arbitrary texcoords later
    if (math::float_eq0(cross(m_positions[1] - m_positions[0], m_positions[2] - m_positions[0])))
    {
        m_corner_count = 0;
    }
}

//------------------------------------------------------------------------------
template<typename Attributes> requires (Attributes::contains(quetzal::geometry::AttributesFlags::Texcoord0))
quetzal::geometry::PolygonMapping<Attributes>::operator bool() const
{
    return m_corner_count >= 3;
}

//------------------------------------------------------------------------------
template<typename Attributes> requires (Attributes::contains(quetzal::geometry::AttributesFlags::Texcoord0))
size_t quetzal::geometry::PolygonMapping<Attributes>::corner_count() const
{
    return m_corner_count;
}

//------------------------------------------------------------------------------
template<typename Attributes> requires (Attributes::contains(quetzal::geometry::AttributesFlags::Texcoord0))
typename quetzal::geometry::PolygonMapping<Attributes>::texcoord_type quetzal::geometry::PolygonMapping<Attributes>::operator()(const point_type& position) const
{
    assert(*this); // Silent wrong texcoords are worse than a stop here
    projected_type p = m_projection(position);
    return m_corner_count == 3 ? affine(p) : bilinear(p);
}

//------------------------------------------------------------------------------
// Barycentric blend of the three corner texcoords. Equivalent to affine_mapping, kept explicit here
// so that the mapping does not depend on the transformation matrix machinery
//------------------------------------------------------------------------------
template<typename Attributes> requires (Attributes::contains(quetzal::geometry::AttributesFlags::Texcoord0))
typename quetzal::geometry::PolygonMapping<Attributes>::texcoord_type quetzal::geometry::PolygonMapping<Attributes>::affine(const projected_type& p) const
{
    projected_vector_type v0 = m_positions[1] - m_positions[0];
    projected_vector_type v1 = m_positions[2] - m_positions[0];
    projected_vector_type v2 = p - m_positions[0];

    value_type d = cross(v0, v1);
    assert(!math::float_eq0(d)); // Guaranteed by the construction check

    value_type b1 = cross(v2, v1) / d;
    value_type b2 = cross(v0, v2) / d;

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
template<typename Attributes> requires (Attributes::contains(quetzal::geometry::AttributesFlags::Texcoord0))
typename quetzal::geometry::PolygonMapping<Attributes>::texcoord_type quetzal::geometry::PolygonMapping<Attributes>::bilinear(const projected_type& p) const
{
    projected_vector_type b = m_positions[1] - m_positions[0];
    projected_vector_type c = m_positions[3] - m_positions[0];
    projected_vector_type d = (m_positions[0] - m_positions[1]) + (m_positions[2] - m_positions[3]);
    projected_vector_type q = p - m_positions[0];

    value_type a2 = cross(c, d);
    value_type a1 = cross(c, b) - cross(q, d);
    value_type a0 = -cross(q, b);

    value_type v = value_type(0);
    if (math::float_eq0(a2))
    {
        assert(!math::float_eq0(a1)); // Degenerate quad, rejected at construction
        v = -a0 / a1;
    }
    else
    {
        value_type discriminant = a1 * a1 - value_type(4) * a2 * a0;
        assert(discriminant >= value_type(0));
        value_type root = std::sqrt(discriminant);

        // Two roots, one of which lies on the patch. Prefer the one within the unit interval
        value_type v0 = (-a1 - root) / (value_type(2) * a2);
        value_type v1 = (-a1 + root) / (value_type(2) * a2);
        v = (v0 >= value_type(0) && v0 <= value_type(1)) ? v0 : v1;
    }

    // u from u(b + dv) = q - cv, using whichever component of the denominator is better conditioned
    projected_vector_type denominator = b + d * v;
    projected_vector_type numerator = q - c * v;
    value_type u = std::abs(denominator.x()) >= std::abs(denominator.y()) ? numerator.x() / denominator.x() : numerator.y() / denominator.y();

    // Evaluate the texcoord patch at the recovered parameters. Along an edge one parameter is 0 or 1
    // and this collapses to a lerp of that edge's two corner texcoords
    return blend(blend(m_texcoords[0], m_texcoords[1], u), blend(m_texcoords[3], m_texcoords[2], u), v);
}

//------------------------------------------------------------------------------
template<typename Attributes> requires (Attributes::contains(quetzal::geometry::AttributesFlags::Texcoord0))
typename quetzal::geometry::PolygonMapping<Attributes>::value_type quetzal::geometry::PolygonMapping<Attributes>::cross(const projected_vector_type& a, const projected_vector_type& b)
{
    return a.x() * b.y() - a.y() * b.x();
}

//------------------------------------------------------------------------------
template<typename Attributes> requires (Attributes::contains(quetzal::geometry::AttributesFlags::Texcoord0))
typename quetzal::geometry::PolygonMapping<Attributes>::texcoord_type quetzal::geometry::PolygonMapping<Attributes>::blend(const texcoord_type& a, const texcoord_type& b, value_type t)
{
    return a + (b - a) * t;
}

#endif // QUETZAL_GEOMETRY_POLYGONMAPPING_HPP
