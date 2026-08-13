#if !defined(QUETZAL_BREP_TEXCOORD_HPP)
#define QUETZAL_BREP_TEXCOORD_HPP
//------------------------------------------------------------------------------
// brep
// texcoord.hpp
//------------------------------------------------------------------------------

#include "Face.hpp"
#include "geometry.hpp"
#include "quetzal/geometry/AxisAlignedBoundingBox.hpp"
#include "quetzal/geometry/Mapping.hpp"
#include "quetzal/geometry/Polygon.hpp"
#include "quetzal/geometry/PolygonMapping.hpp"
#include "quetzal/geometry/Projection.hpp"
#include "quetzal/geometry/transformation_matrix.hpp"
#include <ranges>
#include <cmath>

namespace quetzal::brep
{

    // Surface is unfolded onto a plane
    // not implemented, uses calculate_planar_surface_texcoords instead ...
    template<typename M> requires quetzal::geometry::Textured<typename M::vertex_attributes_type>
    void calculate_surface_texcoords(M& mesh, id_type surface_id);

    // Assumes that all vertices in the surface are coplanar (or already projected onto a plane)
    // the direct uses of planar can be replaced with texcoord assignment from opposite (?) ...
    template<typename M> requires quetzal::geometry::Textured<typename M::vertex_attributes_type>
    void calculate_planar_surface_texcoords(M& mesh, id_type surface_id);

    template<typename M> requires quetzal::geometry::Textured<typename M::vertex_attributes_type>
    void calculate_face_texcoords(M& mesh, id_type face_id);

    template<typename M> requires quetzal::geometry::Textured<typename M::vertex_attributes_type>
    void set_triangular_texcoords(M& mesh, id_type face_id);

    // Calculate texcoords for face or hole from a mapping
    // Lets a caller build the mapping while the reference face is still pristine and apply it to any
    // number of targets afterwards, rather than rederiving it from a face that is being modified
    template<typename FH, typename A> requires quetzal::geometry::Textured<typename FH::traits_type::vertex_attributes_type>
    void interpolate_texcoords(const geometry::PolygonMapping<A>& mapping, FH& target);

    // Calculate texcoords for face or hole by interpolating from texcoords of reference
    template<typename F, typename FH> requires quetzal::geometry::Textured<typename F::traits_type::vertex_attributes_type> && quetzal::geometry::Textured<typename FH::traits_type::vertex_attributes_type>
    void interpolate_texcoords(const F& reference, FH& target);

    template<typename F> requires quetzal::geometry::Textured<typename F::traits_type::vertex_attributes_type>
    typename F::traits_type::vertex_attributes_type::texcoord_type texcoord_vertex_centroid(const F& face);

    template<typename T>
    bool texcoord_eq(const T& a, const T& b);

    // Calculate mapping from positions to texcoords of face
    // Be sure to only call this prior to any operation that modifies the face such that texcoords need to be generated
    template<typename Traits> requires quetzal::geometry::Textured<typename Traits::vertex_attributes_type>
    std::vector<typename Traits::vertex_attributes_type> face_texcoord_corners(const Face<Traits>& face);

    // PolygonMapping requires three vertices for an affine fit, four for bilinear
    template<typename Traits> requires quetzal::geometry::Textured<typename Traits::vertex_attributes_type>
    geometry::PolygonMapping<typename Traits::vertex_attributes_type> position_texcoord_mapping(const Face<Traits>& face);

} // namespace quetzal::brep

//------------------------------------------------------------------------------
template<typename M> requires quetzal::geometry::Textured<typename M::vertex_attributes_type>
void quetzal::brep::calculate_surface_texcoords(M& mesh, id_type surface_id)
{
    calculate_planar_surface_texcoords(mesh, surface_id);
    return;
}

//------------------------------------------------------------------------------
template<typename M> requires quetzal::geometry::Textured<typename M::vertex_attributes_type>
void quetzal::brep::calculate_planar_surface_texcoords(M& mesh, id_type surface_id)
{
    using value_type = M::value_type;
    using texcoord_type = M::vertex_attributes_type::texcoord_type;

    auto& surface = mesh.surface(surface_id);
    geometry::Projection<typename M::vector_traits> projection(surface.attributes().normal());

    std::vector<texcoord_type> points;
    for (const auto& face : surface.faces())
    {
        for (const auto& halfedge : face.halfedges())
        {
            points.emplace_back(projection(halfedge.vertex_attributes().position()));
        }
    }

    geometry::AxisAlignedBoundingBox<typename texcoord_type::traits_type> bounds(points);
    auto lower = bounds.lower();
    auto extent = bounds.extent();

    auto to_texcoord = [&](const texcoord_type& position) -> texcoord_type { return denoise01(texcoord_type{(position.x() - lower.x()) / extent.x(), value_type(1) - (position.y() - lower.y()) / extent.y()}); };

    size_t i = 0;
    for (auto& face : surface.faces())
    {
        for (auto& halfedge : face.halfedges())
        {
            halfedge.vertex_attributes().set_texcoord(to_texcoord(points[i]));
            ++i;
        }

        for (auto& hole : face.holes())
        {
            for (auto& halfedge : hole.halfedges())
            {
                halfedge.vertex_attributes().set_texcoord(to_texcoord(projection(halfedge.vertex_attributes().position())));
            }
        }
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M> requires quetzal::geometry::Textured<typename M::vertex_attributes_type>
void quetzal::brep::calculate_face_texcoords(M& mesh, id_type face_id)
{
    using value_type = M::value_type;
    using texcoord_type = M::vertex_attributes_type::texcoord_type;

    auto& face = mesh.face(face_id);
    geometry::Projection<typename M::vector_traits> projection(face.attributes().normal());

    std::vector<texcoord_type> points;
    for (const auto& halfedge : face.halfedges())
    {
        points.emplace_back(projection(halfedge.vertex_attributes().position()));
    }

    geometry::AxisAlignedBoundingBox<typename texcoord_type::traits_type> bounds(points);
    auto extent = bounds.extent();
    auto to_texcoord = [&](const texcoord_type& position) -> texcoord_type { return denoise01(texcoord_type{(position.x() - bounds.lower().x()) / extent.x(), (bounds.upper().y() - position.y()) / extent.y()}); };

    size_t i = 0;
    for (auto& halfedge : face.halfedges())
    {
        halfedge.vertex_attributes().set_texcoord(to_texcoord(points[i]));
        ++i;
    }

    for (auto& hole : face.holes())
    {
        for (auto& halfedge : hole.halfedges())
        {
            halfedge.vertex_attributes().set_texcoord(to_texcoord(projection(halfedge.vertex_attributes().position())));
        }
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M> requires quetzal::geometry::Textured<typename M::vertex_attributes_type>
void quetzal::brep::set_triangular_texcoords(M& mesh, id_type face_id)
{
    using T = M::value_type;
    using texcoord_type = M::vertex_attributes_type::texcoord_type;

    auto& face = mesh.face(face_id);
    assert(face.halfedge_count() == 3);

    const std::array<texcoord_type, 3> face_texcoords =
    {{
        {T(0), T(1)},
        {T(1), T(1)},
        {T(0.5), T(0)},
    }};

    size_t j = 0;
    for (auto& halfedge : face.halfedges())
    {
        halfedge.vertex_attributes().set_texcoord(face_texcoords[j++]);
    }

    return;
}

//------------------------------------------------------------------------------
template<typename F, typename FH> requires quetzal::geometry::Textured<typename F::traits_type::vertex_attributes_type> && quetzal::geometry::Textured<typename FH::traits_type::vertex_attributes_type>
void quetzal::brep::interpolate_texcoords(const F& reference, FH& target)
{
    interpolate_texcoords(position_texcoord_mapping(reference), target);
    return;
}

//------------------------------------------------------------------------------
template<typename FH, typename A> requires quetzal::geometry::Textured<typename FH::traits_type::vertex_attributes_type>
void quetzal::brep::interpolate_texcoords(const geometry::PolygonMapping<A>& mapping, FH& target)
{
    for (auto& halfedge : target.halfedges())
    {
        halfedge.vertex_attributes().set_texcoord(denoise01(mapping(halfedge.vertex_attributes().position())));
    }

    return;
}

//------------------------------------------------------------------------------
template<typename F> requires quetzal::geometry::Textured<typename F::traits_type::vertex_attributes_type>
typename F::traits_type::vertex_attributes_type::texcoord_type quetzal::brep::texcoord_vertex_centroid(const F& face)
{
    typename F::traits_type::vertex_attributes_type::texcoord_type::vector_type total{};
    auto n = face.halfedge_count();

    for (const auto& halfedge : face.halfedges())
    {
        total += halfedge.vertex_attributes().texcoord().vector();
    }

    return total / F::traits_type::val(n);
}

//------------------------------------------------------------------------------
template<typename T>
bool quetzal::brep::texcoord_eq(const T& a, const T& b)
{
    auto unwrapped = [](auto t) { return t - std::floor(t); };
    return float_eq(a, b) || (math::float_eq(unwrapped(a.x()), unwrapped(b.x())) && math::float_eq(unwrapped(a.y()), unwrapped(b.y())));
}

//------------------------------------------------------------------------------
template<typename Traits> requires quetzal::geometry::Textured<typename Traits::vertex_attributes_type>
std::vector<typename Traits::vertex_attributes_type> quetzal::brep::face_texcoord_corners(const Face<Traits>& face)
{
    using vertex_attributes_type = Traits::vertex_attributes_type;

    assert(face.halfedge_count() >= 3);

    std::vector<typename vertex_attributes_type::point_type> positions;
    for (const auto& halfedge : face.halfedges())
    {
        positions.push_back(vertex_position(halfedge));
    }

    // Reject colinear halfeddges
    std::vector<vertex_attributes_type> avs;
    size_t n = positions.size();
    size_t i = 0;

    for (const auto& halfedge : face.halfedges())
    {
        if (!colinear(positions[(i + n - 1) % n], positions[i], positions[(i + 1) % n]))
        {
            avs.push_back(halfedge.vertex_attributes());
        }

        ++i;
    }

    return avs;
}

//------------------------------------------------------------------------------
template<typename Traits> requires quetzal::geometry::Textured<typename Traits::vertex_attributes_type>
quetzal::geometry::PolygonMapping<typename Traits::vertex_attributes_type> quetzal::brep::position_texcoord_mapping(const Face<Traits>& face)
{
    return {face_texcoord_corners(face), face.attributes().normal()};
}

#endif // QUETZAL_BREP_TEXCOORD_HPP
