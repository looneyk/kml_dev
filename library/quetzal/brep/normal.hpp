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
    typename Traits::vector_type vertex_normal(const Halfedge<Traits>& halfedge);

    // The normal vector taking into account only the single face containing that vertex
    template<typename Traits>
    std::tuple<typename Traits::vector_type, typename Traits::value_type> vertex_normal_angle(const Halfedge<Traits>& halfedge);

    // The normal vector taking into account all of the faces that meet at that vertex and that are in the vertex's surface
    template<typename Traits>
    typename Traits::vector_type surface_normal(const Vertex<Traits>& vertex);

    // Calculate normals for each face
    // Works with Mesh, Submesh, Surface
    template<typename U>
    void calculate_face_normals(U& u);

    // Calculate vertex normals for each face in each surface in the mesh/submesh
    // Works with Mesh, Submesh
    template<typename U>
    void calculate_surface_vertex_normals(U& u);

    // Calculate smooth surface normals for each vertex in each face in the surface
    // Weighted by the angle contributed by each surface face
    // Assumes that the face normals have been precalculated and are valid
    template<typename Traits>
    void calculate_surface_vertex_normals(Surface<Traits>& surface);

    // Works with Mesh, Submesh, Surface
    // The above without precalculated face normal optimizations and denoising, does not require or calculate face normal ...
    template<typename U>
    void calculate_surface_vertex_normals_(U& u);

    // Calculates all face normals and then all smooth surface vertex normals
    template<typename U>
    void calculate_normals(U& u);

    // Set the surface, face, and vertex normals to the common value given for a face that is the sole face in a surface
    // Calculate it if it is not given
    // Only triangulate_central_vertex is using the explicit normal functionality here, 
    template<typename F>
    void set_facet_normal(F& face, const std::optional<const typename F::attributes_type::vector_type>& normal = std::nullopt);

} // namespace quetzal::brep

//------------------------------------------------------------------------------
template<typename Traits>
typename Traits::vector_type quetzal::brep::vertex_normal(const Halfedge<Traits>& halfedge)
{
    auto v_next = direction(halfedge);
    auto v_prev = -direction(halfedge.prev());
    return normalize(denoise0(cross(v_next, v_prev)));
}

//------------------------------------------------------------------------------
template<typename Traits>
std::tuple<typename Traits::vector_type, typename Traits::value_type> quetzal::brep::vertex_normal_angle(const Halfedge<Traits>& halfedge)
{
    auto v_next = direction(halfedge);
    auto v_prev = -direction(halfedge.prev());
    return { normalize(denoise0(cross(v_next, v_prev))), angle_between_unit(v_next, v_prev)};
}

//----------------------------------------------------------------------------------
template<typename Traits>
typename Traits::vector_type quetzal::brep::surface_normal(const Vertex<Traits>& vertex)
{
    id_type surface_id = vertex.halfedge().surface_id();

    auto position = vertex.attributes().position();
    typename Traits::vector_type weighted_sum;
//std::cout << "surface_normal " << vertex.id() << " (" << position << ')' << std::endl;

    for (auto& halfedge : vertex.halfedges())
    {
//        assert(float_eq(halfedge.vertex_attributes().position(), position));
        assert(halfedge.vertex_attributes().position() == position);

        if (halfedge.surface_id() != surface_id)
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
template<typename U>
void quetzal::brep::calculate_face_normals(U& u)
{
    for (auto& face : u.faces())
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
                if (h.surface_id() != surface.id())
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
template<typename U>
void quetzal::brep::calculate_surface_vertex_normals_(U& u)
{
    u.reset_all();
    for (auto& face : u.faces())
    {
        for (auto& halfedge : face.halfedges())
        {
            auto& vertex = halfedge.vertex();
            if (vertex.marked())
            {
                continue;
            }

            auto normal = surface_normal(vertex);
            for (auto& h : vertex.halfedges())
            {
                if (h.surface_id() != halfedge.surface.id())
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
template<typename U>
void quetzal::brep::calculate_normals(U& u)
{
    calculate_face_normals(u);
    calculate_surface_vertex_normals(u);
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
