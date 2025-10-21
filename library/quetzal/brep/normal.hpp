#if !defined(QUETZAL_BREP_NORMAL_HPP)
#define QUETZAL_BREP_NORMAL_HPP
//------------------------------------------------------------------------------
// brep
// normal.hpp
//------------------------------------------------------------------------------

#include "Surface.hpp"
#include "Vertex.hpp"
#include "geometry.hpp"
#include <optional>

namespace quetzal::brep
{

    // The normal vector taking into account only the single face containing that vertex
    template<typename Traits>
    typename Traits::vector_type vertex_normal(const Vertex<Traits>& vertex);

    // The normal vector taking into account all of the faces that meet at that vertex and that are in the vertex's surface
    template<typename Traits>
    typename Traits::vector_type surface_normal(const Vertex<Traits>& vertex);

    // Calculate face normals for each face in the surface
    template<typename Traits>
    void calculate_face_normals(Surface<Traits>& surface);

    // Calculate vertex normals for the each face in each surface in the mesh/submesh
    // Works with Mesh, Submesh
    template<typename U>
    void calculate_surface_vertex_normals(U& u);

    // Calculate smooth surface normals for each vertex in each face in the surface
    // Weighted by the angle contributed by each surface face
    // Assumes that the face normals have been precalculated and are valid
    template<typename Traits>
    void calculate_surface_vertex_normals(Surface<Traits>& surface);

    // Calculates all face normals and then all smooth surface vertex normals
    // add Mesh/Submesh version of this as above? ...
    template<typename S>
    void calculate_normals(S& surface);

    // Set the face, surface, and vertex normals to the common value given for a face that is the sole face in a surface
    // Calculate it if it is not given
    template<typename F>
    void set_facet_normal(F& face, const std::optional<const typename F::attributes_type::vector_type>& normal = std::nullopt);

} // namespace quetzal::brep

//------------------------------------------------------------------------------
template<typename Traits>
typename Traits::vector_type quetzal::brep::vertex_normal(const Vertex<Traits>& vertex)
{
    const auto& halfedge = vertex.halfedge();
    const auto& position = halfedge.vertex_attributes().position();
    const auto& position_next = halfedge.next().vertex_attributes().position();
    const auto& position_prev = halfedge.prev().vertex_attributes().position();
    return normalize(denoise0(cross(position_next - position, position_prev - position)));
}

//----------------------------------------------------------------------------------
template<typename Traits>
typename Traits::vector_type quetzal::brep::surface_normal(const Vertex<Traits>& vertex)
{
    id_type idSurface = vertex.halfedge().surface_id();

    auto position = vertex.attributes().position();
    typename Traits::vector_type weighted_sum;
//std::cout << "surface_normal " << vertex.id() << " (" << position << ')' << std::endl;

    for (auto& halfedge : vertex.halfedges())
    {
//        assert(float_eq(halfedge.vertex_attributes().position(), position));
        assert(halfedge.vertex_attributes().position() == position);

        if (halfedge.surface_id() != idSurface)
        {
            continue;
        }

// just use halfedge.face_attributes().normal(), but vector calculation or unnormalized cross product is still needed for weight ...
//auto pn = halfedge.next().vertex_attributes().position();
//auto pp = halfedge.prev().vertex_attributes().position();
//std::cout << "    (" << pn << ") (" << pp << ')' << std::endl;
        auto v_next = normalize(halfedge.next().vertex_attributes().position() - position);
        auto v_prev = normalize(halfedge.prev().vertex_attributes().position() - position);
// can't do this until the non-planar face issue is resolved ...
//        auto normal = halfedge.face_attributes().normal();
        auto normal = normalize(cross(v_next, v_prev));
        auto weight = angle_between_unit(v_next, v_prev);
        weighted_sum += weight * normal;
    }

//    return normalize(weighted_sum);
    return normalize(denoise0(weighted_sum));
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::brep::calculate_face_normals(Surface<Traits>& surface)
{
    for (auto& face : surface.faces())
    {
        face.attributes().set_normal(geometry::normal(face));
    }

    return;
}

//------------------------------------------------------------------------------
template<typename U>
void quetzal::brep::calculate_surface_vertex_normals(U& u)
{
    for (auto& surface : u.surfaces())
    {
        calculate_surface_vertex_normals(surface);
    }

    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::brep::calculate_surface_vertex_normals(Surface<Traits>& surface)
{
    if (surface.face_count() == 1)
    {
        auto& face = surface.faces().front();
        const auto& normal = face.attributes().normal();

        for (auto& halfedge : face.halfedges())
        {
            halfedge.vertex_attributes().set_normal(normal);
        }

        return;
    }

    surface.reset_all();

    for (auto& face : surface.faces())
    {
        auto face_normal = face.attributes().normal();

        for (auto& halfedge : face.halfedges())
        {
            auto& vertex = halfedge.vertex();
            if (vertex.marked())
            {
                continue;
            }

            auto normal = surface_normal(vertex);
            if (float_eq(normal, face_normal))
            {
                normal = face_normal;
            }

            for (auto& h : vertex.halfedges())
            {
                if (h.face().surface_id() != surface.id())
                {
                    continue;
                }

                h.vertex_attributes().set_normal(normal);
                h.vertex().set_marked();
            }
        }
    }

    return;
}

//------------------------------------------------------------------------------
template<typename S>
void quetzal::brep::calculate_normals(S& surface)
{
    calculate_face_normals(surface);
    calculate_surface_vertex_normals(surface);
    return;
}

//------------------------------------------------------------------------------
template<typename F>
void quetzal::brep::set_facet_normal(F& face, const std::optional<const typename F::attributes_type::vector_type>& normal)
{
    assert(face.surface().face_count() == 1);

    auto facet_normal = normal.value_or(geometry::normal(face));

    face.attributes().set_normal(facet_normal);
    face.surface().attributes().set_normal(facet_normal);

    for (auto& halfedge : face.halfedges())
    {
        halfedge.vertex_attributes().set_normal(facet_normal);
    }

    return;
}

#endif // QUETZAL_BREP_NORMAL_HPP
