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
#include "quetzal/geometry/Projection.hpp"
#include "quetzal/geometry/transformation_matrix.hpp"
#include <ranges>

/*
Note that for these functions, the verttex attributes texcoord check should be done by the caller:
    if constexpr (M::vertex_attributes_type::contains(geometry::AttributesFlags::Texcoord0))
    and/or add constraints here ...
*/

namespace quetzal::brep
{

    // Surface is unfolded onto a plane
    // not implemented, uses calculate_planar_surface_texcoords instead ...
    template<typename M>
    void calculate_surface_texcoords(M& mesh, id_type surface_id);

    // Assumes that all vertices in the surface are coplanar (or already projected onto a plane)
    // the direct uses of planar can be replaced with texcoord assignment from opposite (?) ...
    template<typename M>
    void calculate_planar_surface_texcoords(M& mesh, id_type surface_id);

    template<typename M>
    void calculate_face_texcoords(M& mesh, id_type face_id);

    template<typename M>
    void set_triangular_texcoords(M& mesh, id_type face_id);

    // Calculate texcoords for face or hole by interpolating from texcoords of reference
    template<typename F, typename FH>
    void interpolate_texcoords(const F& reference, FH& target);

    template<typename F>
    typename F::traits_type::vertex_attributes_type::texcoord_type texcoord_vertex_centroid(const F& face);

    // Calculate mapping from positions to texcoords of face
    // This assumes that the set of positions is coplanar in 3D space and thus can be reduced to a planar projection
    template<typename Traits>
    geometry::Mapping<typename Traits::vertex_attributes_type::point_type, typename Traits::vertex_attributes_type::texcoord_type, geometry::Projection<typename Traits::vertex_attributes_type::traits_type>> position_texcoord_mapping(const Face<Traits>& face);

} // namespace quetzal::brep

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::calculate_surface_texcoords(M& mesh, id_type surface_id)
{
    calculate_planar_surface_texcoords(mesh, surface_id);
    return;
}

//------------------------------------------------------------------------------
template<typename M>
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
template<typename M>
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
template<typename M>
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
template<typename F, typename FH>
void quetzal::brep::interpolate_texcoords(const F& reference, FH& target)
{
    auto mapping = position_texcoord_mapping(reference);

    for (auto& halfedge : target.halfedges())
    {
        halfedge.vertex_attributes().set_texcoord(denoise01(mapping(halfedge.vertex_attributes().position())));
    }

    return;
}

//------------------------------------------------------------------------------
template<typename F>
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
template<typename Traits>
quetzal::geometry::Mapping<typename Traits::vertex_attributes_type::point_type, typename Traits::vertex_attributes_type::texcoord_type, quetzal::geometry::Projection<typename Traits::vertex_attributes_type::traits_type>> quetzal::brep::position_texcoord_mapping(const Face<Traits>& face)
{
    using vertex_attributes_type = Traits::vertex_attributes_type;
    using point_type = vertex_attributes_type::point_type;
    using projection_type = geometry::Projection<typename vertex_attributes_type::traits_type>;
    using projected_type = projection_type::projected_type;

    assert(face.halfedge_count() >= 3);
    size_t n = face.halfedge_count() > 3 ? 4 : 3;

    auto avs = std::ranges::take_view(face.halfedges(), n) | std::views::transform([](const auto& halfedge) { return halfedge.vertex_attributes(); });
    auto positions = std::ranges::to<std::vector>(avs | std::views::transform([](const auto& av) -> point_type { return av.position(); }));
    auto texcoords = std::ranges::to<std::vector>(avs | std::views::transform([](const auto& av) -> projected_type { return av.texcoord(); }));

    projection_type projection(face.attributes().normal());

    return {positions, texcoords, projection};
}

#endif // QUETZAL_BREP_TEXCOORD_HPP
