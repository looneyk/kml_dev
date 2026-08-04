#if !defined(QUETZAL_BREP_TRIANGULATION_HPP)
#define QUETZAL_BREP_TRIANGULATION_HPP
//------------------------------------------------------------------------------
// brep
// triangulation.hpp
//------------------------------------------------------------------------------

#include "id.hpp"
#include "geometry.hpp"
#include "mesh_util.hpp"
#include "normal.hpp"
#include "texcoord.hpp"
#include "Mesh.hpp"
#include "Submesh.hpp"
#include "quetzal/geometry/Point.hpp"
#include "quetzal/geometry/Projection.hpp"
#include "quetzal/math/floating_point.hpp"
#include "quetzal/triangulation/triangulation.hpp"
#include <ranges>
#include <cassert>

namespace quetzal::brep
{

    // mesh, id is needed for addressing at the surface and face levels here since the operation may cause iterators to be invalidated (surface, only in the faceted case)
    // These use only existing vertex positions, with the exception of the central_vertex variants

    template<typename Traits>
    void triangulate(Mesh<Traits>& mesh);

    template<typename Traits>
    void triangulate(Submesh<Traits>& submesh);

    template<typename M>
    void triangulate_face_quad(M& mesh, id_type face_id);

    // Uses only existing vertex positions
    template<typename M>
    void triangulate_face_ear(M& mesh, id_type face_id);

    // Uses only existing vertex positions
    template<typename M>
    void triangulate_face_cdt(M& mesh, id_type face_id);

    // Delaunay triangulation of all faces in a surface
    template<typename M>
    void triangulate_surface(M& mesh, id_type submesh_id, const std::string& name);

    // Delaunay triangulation of all faces in a surface
    template<typename M>
    void triangulate_surface(M& mesh, id_type surface_id);

    // When smooth is false, containing surface should consist of only this single face
    // Faces should be convex polygons
    // Undefined results otherwise

    // Adds a central vertex and a face for each original face edge of each face in the submesh
    template<typename Traits>
    void triangulate_central_vertices(Submesh<Traits>& submesh, bool smooth = true, bool reject_triangular = true);

    // Adds a central vertex and a face for each original face edge of each face in the surface
    template<typename M>
    void triangulate_central_vertices(M& mesh, id_type surface_id, bool smooth = true, bool reject_triangular = true);

    // Adds a central vertex and a face for each original face edge
    // Central vertex position is calculated using vertex_centroid
    template<typename M>
    void triangulate_central_vertex(M& mesh, id_type face_id, bool smooth = true);

    // Adds a new vertex at position and a face for each original face edge
    // Surface vertex normal calculation is deferred if this face is part of a larger surface, they should only be calculated once all face modifications have been completed
    template<typename M>
    void triangulate_central_vertex(M& mesh, id_type face_id, const typename M::point_type& position, bool smooth = true);

} // namespace quetzal::brep

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::brep::triangulate(Mesh<Traits>& mesh)
{
    size_t face_count_orig = mesh.face_store_count();
    for (size_t i = 0; i < face_count_orig; ++i)
    {
        const auto& face = mesh.face_store()[i];
        if (face.deleted() || (face.halfedge_count() == 3 && face.hole_count() == 0))
        {
            continue;
        }

        if (face.halfedge_count() == 4 && face.hole_count() == 0)
        {
            triangulate_face_quad(mesh, i);
            continue;
        }

        triangulate_face_cdt(mesh, i);
    }

    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::brep::triangulate(Submesh<Traits>& submesh)
{
    auto& mesh = submesh.mesh();

    auto face_ids = submesh.face_ids();
    for (id_type face_id : face_ids)
    {
        const auto& face = mesh.face_store()[face_id];
        if (face.halfedge_count() == 3 && face.hole_count() == 0)
        {
            continue;
        }

        if (face.halfedge_count() == 4 && face.hole_count() == 0)
        {
            triangulate_face_quad(mesh, face_id);
            continue;
        }

        triangulate_face_cdt(mesh, face_id);
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::triangulate_face_quad(M& mesh, id_type face_id)
{
    const auto& face = mesh.face(face_id);
    assert(face.halfedge_count() == 4);
    assert(face.hole_count() == 0);

/* this should fix concave splits, but looks like there is still a problem with reflex_vertex ...
    auto reflex_halfedge_id = reflex_vertex(face);
    if (reflex_halfedge_id)
    {
        id_type halfedge_id = *reflex_halfedge_id;
        split_face(mesh, halfedge_id, mesh.halfedge(halfedge_id).next().next_id());
        return;
    }
*/
    id_type halfedge_ids[4];
    typename M::point_type positions[4];

    for (const auto& e : std::views::zip(face.halfedges(), halfedge_ids, positions))
    {
        const auto& halfedge = get<0>(e);
        get<1>(e) = halfedge.id();
        get<2>(e) = halfedge.vertex_attributes().position();
    }

    typename M::value_type d02 = (positions[0] - positions[2]).norm_squared();
    typename M::value_type d13 = (positions[1] - positions[3]).norm_squared();

    // split using shortest diagonal
    if (d02 < d13)
    {
        split_face(mesh, halfedge_ids[0], halfedge_ids[2]);
    }
    else
    {
        split_face(mesh, halfedge_ids[1], halfedge_ids[3]);
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::triangulate_face_ear(M& mesh, id_type face_id)
{
    const auto& face = mesh.face(face_id);
    assert(face.halfedge_count() > 3);
    assert(face.hole_count() == 0);

// mark reflex vertices ...
    auto normal = face.attributes().normal();
    size_t edge_count = face.halfedge_count();
    size_t n = 0; // Counts number of failed attempts to find ear vertex

    id_type halfedge_id = face.halfedge_id();
    while (mesh.halfedge(halfedge_id).face().halfedge_count() > 3)
    {
        id_type halfedge_ids_next = mesh.halfedge(halfedge_id).next_id();
        // a bit better by advancing to next vertex; this should be done away with and instead use better triangulation algorithm ...
        if (convex(mesh.halfedge(halfedge_ids_next).face()))
        {
            halfedge_ids_next = mesh.halfedge(halfedge_ids_next).next_id();
        }

        if (ear(mesh.halfedge(halfedge_id)))
        {
            split_face(mesh, mesh.halfedge(halfedge_id).prev_id(), mesh.halfedge(halfedge_id).next_id());
            // update status of prev and next vertices ...
            n = 0;
        }

        halfedge_id = halfedge_ids_next;
        assert(++n < edge_count);
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::triangulate_face_cdt(M& mesh, id_type face_id)
{
    const auto& face = mesh.face(face_id);
    auto af = face.attributes();
    auto normal = af.normal();

    geometry::Projection<typename M::vector_traits> projection(normal);

    auto to_p2t_polygon = [&projection]<typename T>(const T& t)
    {
        std::vector<p2t::Point> polygon;
        for (const auto& halfedge : t.halfedges())
        {
            const auto position = projection(halfedge.vertex_attributes().position());
            polygon.emplace_back(halfedge.id(), position.x(), position.y());
        }
        return polygon;
    };

    auto polygon = to_p2t_polygon(face);
    p2t::CDT cdt(polygon);

    for (const auto& hole : face.holes())
    {
        polygon = std::move(to_p2t_polygon(hole));
        cdt.AddHole(polygon);
    }

    cdt.Triangulate();

    std::vector<p2t::Triangle> triangles = cdt.GetTriangles();
    assert(!triangles.empty());

    // Delete original face leaving its halfedges, vertices, and surface

    id_type submesh_id = face.submesh_id();
    if (submesh_id != nullid)
    {
        mesh.submesh(submesh_id).unlink_face(face_id);
    }

    id_type surface_id = face.surface_id();
    if (surface_id != nullid)
    {
        mesh.surface(surface_id).unlink_face(face_id);
    }

    mesh.face(face_id).set_deleted();

    id_type nh_orig = mesh.halfedge_store_count();
    id_type nh = nh_orig;

    // Store first halfedge of each new pair for partner lookup
    std::map<std::array<id_type, 2>, id_type> edges; // ...

    for (size_t i = 0; i < triangles.size(); ++i)
    {
        p2t::Triangle& triangle = triangles[i];

        id_type halfedge_ids_orig[3];
        id_type halfedge_ids_face[3];

        for (size_t j = 0; j < 3; ++j)
        {
            size_t j_next = (j + 1) % 3;

            id_type halfedge_id = triangle.GetPoint(j)->m_id;
            id_type halfedge_id_next = triangle.GetPoint(j_next)->m_id;

            halfedge_ids_orig[j] = halfedge_id;

            if (halfedge_id_next == mesh.halfedge(halfedge_id).next_id()) // Original edge
            {
                halfedge_ids_face[j] = halfedge_id;
            }
            else
            {
                halfedge_ids_face[j] = nh++;
            }
        }

        id_type face_id_triangle = mesh.create_face(surface_id, halfedge_ids_face[0], af);

        for (size_t j = 0; j < 3; ++j)
        {
            size_t j_next = (j + 1) % 3;
            size_t j_prev = (j + 2) % 3;

            id_type halfedge_id = halfedge_ids_face[j];
            id_type halfedge_id_next = halfedge_ids_face[j_next];
            id_type halfedge_id_prev = halfedge_ids_face[j_prev];

            if (halfedge_id < nh_orig) // Reuse original halfedge and vertex
            {
                auto& halfedge = mesh.halfedge(halfedge_id);
                halfedge.set_next_id(halfedge_id_next);
                halfedge.set_prev_id(halfedge_id_prev);
                halfedge.set_face_id(face_id_triangle);
            }
            else
            {
                auto i_edge = edges.find({halfedge_ids_orig[j_next], halfedge_ids_orig[j]});
                id_type partner_id = i_edge != edges.end() ? i_edge->second : nullid;
                if (partner_id != nullid)
                {
                    mesh.halfedge(partner_id).set_partner_id(halfedge_id);
                    edges.erase({halfedge_ids_orig[j_next], halfedge_ids_orig[j]});
                }
                else
                {
                    edges[{halfedge_ids_orig[j], halfedge_ids_orig[j_next]}] = halfedge_id;
                }

                assert(halfedge_id == mesh.halfedge_store_count());

                mesh.create_halfedge_vertex(partner_id, halfedge_id_next, halfedge_id_prev, face_id_triangle, mesh.halfedge(halfedge_ids_orig[j]).vertex_attributes());
            }
        }
    }

    assert(edges.empty());
    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::triangulate_surface(M& mesh, id_type submesh_id, const std::string& name)
{
    triangulate_surface(mesh, mesh.surface_id(submesh_id, name));
    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::triangulate_surface(M& mesh, id_type surface_id)
{
    assert(surface_id != nullid);

    auto& surface = mesh.surface(surface_id);
    auto face_ids = surface.face_ids();
    for (const auto& face_id : face_ids)
    {
        if (mesh.face(face_id).halfedge_count() > 3)
        {
            triangulate_face_cdt(mesh, face_id);
        }
    }

    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::brep::triangulate_central_vertices(Submesh<Traits>& submesh, bool smooth, bool reject_triangular)
{
    auto surface_ids = submesh.surface_ids();
    for (id_type surface_id : surface_ids)
    {
        triangulate_central_vertices(submesh.mesh(), surface_id, smooth, reject_triangular);
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::triangulate_central_vertices(M& mesh, id_type surface_id, bool smooth, bool reject_triangular)
{
    auto face_ids = mesh.surface(surface_id).face_ids();
    for (id_type face_id : face_ids)
    {
        if (reject_triangular && mesh.face(face_id).halfedge_count() == 3)
        {
            continue;
        }
        triangulate_central_vertex(mesh, face_id, smooth);
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::triangulate_central_vertex(M& mesh, id_type face_id, bool smooth)
{
    triangulate_central_vertex(mesh, face_id, vertex_centroid(mesh.face(face_id)), smooth);
    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::triangulate_central_vertex(M& mesh, id_type face_id, const typename M::point_type& position, bool smooth)
{
    const auto& face = mesh.face(face_id);
    assert(face.halfedge_count() >= 3);
    assert(face.holes().empty());

    static std::optional<typename M::vertex_attributes_type> av_reference; // Allows for attribute values to avoid precision discrepancies

    typename M::vertex_attributes_type av;
    av.set_position(denoise0(position));
    av.set_normal(face.attributes().normal()); // assuming smooth and planar at this stage ...

    if constexpr (geometry::Textured<typename M::vertex_attributes_type>)
    {
        if (smooth)
        {
            auto mapping = position_texcoord_mapping(face);
            auto texcoord = denoise01(mapping(av.position()));
            if (!av_reference)
            {
                av_reference.emplace();
                av_reference->set_texcoord(texcoord);
            }
            else
            {
                if (float_eq(texcoord, av_reference->texcoord()))
                {
                    texcoord = av_reference->texcoord();
                }
            }
            av.set_texcoord(texcoord);
        }
    }

    size_t n = face.halfedge_count();

    id_type surface_id0 = face.surface_id();
    if (!smooth)
    {
        id_type submesh_id = face.submesh_id();
        surface_id0 = mesh.surface_store_count();
        for (size_t i = 0; i < n; ++i)
        {
            mesh.create_surface(submesh_id, std::format("{}_{}", face.surface().name(), i)); // create separate surfaces for each facet
        }
    }

    auto surface_id = surface_id0;
    auto af = face.attributes();

    id_type halfedge_id = face.halfedge_id();

    for (size_t j = 0; j < n; ++j)
    {
        id_type nh = mesh.halfedge_store_count();
        id_type halfedge_id_next = mesh.halfedge(halfedge_id).next_id();

        auto av1 = mesh.halfedge(halfedge_id).next().vertex_attributes();

        id_type triangle_face_id = mesh.create_face(surface_id, halfedge_id, af);
        id_type halfedge_id0 = mesh.create_halfedge_vertex((j == 0 ? nh - 1 + 2 * n : nh - 1), halfedge_id, nh + 1, triangle_face_id, av);
        id_type halfedge_id1 = mesh.create_halfedge_vertex((j == n - 1 ? nh + 2 - 2 * n : nh + 2), nh, halfedge_id, triangle_face_id, av1);

        mesh.halfedge(halfedge_id).set_next_id(halfedge_id1);
        mesh.halfedge(halfedge_id).set_prev_id(halfedge_id0);
        mesh.halfedge(halfedge_id).set_face_id(triangle_face_id);

        auto& triangle_face = mesh.face(triangle_face_id);

        auto normal = geometry::normal(triangle_face);
        if (float_eq(normal, af.normal()))
        {
            normal = af.normal();
        }

        if (smooth)
        {
            triangle_face.attributes().set_normal(normal);
        }
        else
        {
            set_facet_normal(triangle_face, normal);

            if constexpr (geometry::Textured<typename M::vertex_attributes_type>)
            {
                set_triangular_texcoords(mesh, triangle_face_id);
            }

            ++surface_id;
        }

        halfedge_id = halfedge_id_next;
    }

    if (smooth)
    {
        if (mesh.surface(surface_id0).face_count() == 1)
        {
            calculate_surface_vertex_normals(mesh.surface(surface_id0));
        }
    }

    // All of original face's halfedges and vertices have been repurposed
    mesh.unlink_face(face_id);
    mesh.face(face_id).set_deleted();
    return;
}

#endif // QUETZAL_BREP_TRIANGULATION_HPP
