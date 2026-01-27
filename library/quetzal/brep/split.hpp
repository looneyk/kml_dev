#if !defined(QUETZAL_BREP_SPLIT_HPP)
#define QUETZAL_BREP_SPLIT_HPP
//------------------------------------------------------------------------------
// brep
// split.hpp
//------------------------------------------------------------------------------

#include "Submesh.hpp"
#include "geometry.hpp"
#include "mesh_util.hpp"
#include "quetzal/geometry/HalfSpace.hpp"
#include "quetzal/geometry/Line.hpp"
#include "quetzal/geometry/intersect.hpp"
#include <string>
#include <queue>

namespace quetzal::brep
{

    template<typename M>
    void split(M& mesh, const geometry::HalfSpace<typename M::vector_traits>& halfspace, const std::string& surface_name = "split");

    template<typename Traits>
    void split(Submesh<Traits>& submesh, const geometry::HalfSpace<typename Traits::vector_traits>& halfspace, const std::string& surface_name = "split");

    // Set all connected surfaces and faces to a new submesh
    template<typename M>
    void set_submesh_connected(M& mesh, id_type face_id, id_type submesh_id);

} // namespace quetzal::brep

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::split(M& mesh, const geometry::HalfSpace<typename M::vector_traits>& halfspace, const std::string& surface_name)
{
    mesh.reset_all(); // for_each_vertex_position also does this ...

    // Mark exterior vertices
    // Vertices in halfspace border plane are considered interior

    bool exterior = false;
    for_each_vertex_position(mesh,
        [&exterior, &halfspace](const auto& vertex) -> bool
        {
            exterior = halfspace.exterior(vertex.attributes().position());
            return true;
        },
        [&exterior](auto& vertex) -> void
        {
            vertex.set_marked(exterior);
            return;
        }
    );

    // Split intersecting faces

    id_type face_count_orig = mesh.face_store_count();
    for (size_t nFace = 0; nFace < face_count_orig; ++nFace)
    {
        auto& face = mesh.face_store()[nFace];
        if (face.deleted())
        {
            continue;
        }

        bool all_interior = true;
        bool all_exterior = true;
        for (const auto& halfedge : face.halfedges())
        {
            if (halfedge.vertex().marked())
            {
                all_interior = false;
            }
            else
            {
                all_exterior = false;
            }
        }

        if (all_interior)
        {
            continue;
        }
        if (all_exterior)
        {
            face.set_marked();
            continue;
        }

        id_type halfedge_id_transition_out = nullid;
        id_type halfedge_id_transition_in = nullid;

        for (const auto& halfedge : face.halfedges())
        {
            bool marked0 = halfedge.vertex().marked();
            bool marked1 = halfedge.next().vertex().marked();

            if (marked0 && !marked1)
            {
                halfedge_id_transition_in = halfedge.id();
            }
            else if (!marked0 && marked1)
            {
                halfedge_id_transition_out = halfedge.id();
            }
        }

        assert(halfedge_id_transition_out != nullid);
        assert(halfedge_id_transition_in != nullid);

        // Don't split if non-exterior point already intersects halfspace plane
        if (!halfspace.boundary(mesh.halfedge(halfedge_id_transition_in).next().vertex_attributes().position()))
        {
            auto line = to_line(mesh.halfedge(halfedge_id_transition_in));
            auto intersection = geometry::intersection(line, halfspace.plane());
            assert(intersection.locus() == geometry::Locus::Point);
            auto position = intersection.point();
            split_edge(mesh, halfedge_id_transition_in, position);
        }
        else if (mesh.halfedge(halfedge_id_transition_in).next().id() == halfedge_id_transition_out)
        {
            // The only halfspace plane intersection is this vertex, so this is actually an exterior face, mark it as such
            mesh.halfedge(halfedge_id_transition_out).vertex().set_marked();
            face.set_marked();
            continue;
        }
        else if (halfspace.boundary(mesh.halfedge(halfedge_id_transition_out).vertex_attributes().position()))
        {
            // Both inward and outward transitioning halfedges intersect with the halfspace plane
            // If all vertices between halfedge_id_transition_in and halfedge_id_transition_out are in halfspace plane or exterior, then this is an exterior face

            exterior = true; // reused ...
            for (id_type halfedge_id = mesh.halfedge(halfedge_id_transition_in).next().id(); halfedge_id != halfedge_id_transition_out; halfedge_id = mesh.halfedge(halfedge_id).next_id())
            {
                bool incident = halfspace.boundary(mesh.halfedge(halfedge_id).vertex_attributes().position());
                if (!mesh.halfedge(halfedge_id).marked() && !incident)
                {
                    exterior = false;
                    break;
                }
            }

            if (exterior)
            {
                for (id_type halfedge_id = mesh.halfedge(halfedge_id_transition_in).next().id(); halfedge_id != halfedge_id_transition_out; halfedge_id = mesh.halfedge(halfedge_id).next_id())
                {
                    mesh.halfedge(halfedge_id).vertex().set_marked();
                }

                mesh.halfedge(halfedge_id_transition_out).vertex().set_marked();
                face.set_marked();
                continue;
            }
        }

        halfedge_id_transition_in = mesh.halfedge(halfedge_id_transition_in).next_id(); // Adjust so vertex is on halfspace plane
        assert(halfspace.boundary(mesh.halfedge(halfedge_id_transition_in).vertex_attributes().position()));

        assert(!mesh.halfedge(halfedge_id_transition_in).vertex().marked());
        assert(!mesh.halfedge(halfedge_id_transition_out).vertex().marked());

        if (!halfspace.boundary(mesh.halfedge(halfedge_id_transition_out).vertex_attributes().position()))
        {
            auto line = to_line(mesh.halfedge(halfedge_id_transition_out));
            auto intersection = geometry::intersection(line, halfspace.plane());
            assert(intersection.locus() == geometry::Locus::Point);
            auto position = intersection.point();
            split_edge(mesh, halfedge_id_transition_out, position);

            halfedge_id_transition_out = mesh.halfedge(halfedge_id_transition_out).next_id(); // Adjust so vertex is on halfspace plane
        }

        mesh.halfedge(halfedge_id_transition_out).vertex().set_marked();

        assert(halfspace.boundary(mesh.halfedge(halfedge_id_transition_out).vertex_attributes().position()));

        // Split face into interior and exterior faces
        split_face(mesh, halfedge_id_transition_in, halfedge_id_transition_out);
        mesh.halfedge(halfedge_id_transition_out).prev().vertex().set_marked();
        mesh.halfedge(halfedge_id_transition_out).face().set_marked();

        // ** instead of marking here, split, uncouple, and add directly to border_halfedges ...
    }

    // Delete all vertices, edges, and faces that are exterior to the halfspace plane, store remaining border halfedges

    // border_halfedges[halfedge_id] = surface_id
    BorderHalfedges border_halfedges0;
    BorderHalfedges border_halfedges1;

    for (auto& face : mesh.faces())
    {
        if (face.marked())
        {
            for (auto& halfedge : face.halfedges())
            {
                assert(halfedge.vertex().marked());
            }

            continue;
        }

        for (auto& halfedge : face.halfedges())
        {
            assert(!halfedge.vertex().marked());
            assert(!halfedge.border());

            if (halfedge.partner().vertex().marked())
            {
                border_halfedges0.insert({halfedge.id(), face.submesh_id()});
                border_halfedges1.insert({halfedge.partner_id(), face.submesh_id()});
                halfedge.partner().set_border();
                halfedge.set_border();
            }
        }
    }

    assert(border_halfedges0.size() == border_halfedges1.size());
    if (border_halfedges0.empty()) // No intersection
    {
        return;
    }

    // Close off borders and assign each to a new submesh
    // This will still potentially create disconnected submeshes with one section per surface, need better naming ...

    auto surface_ids = close_border(mesh, border_halfedges0, surface_name, halfspace.plane().normal());
    for (id_type surface_id : surface_ids)
    {
        id_type submesh_id = mesh.get_submesh_id(mesh.surface(surface_id).submesh().name() + "_0");
        set_submesh_connected(mesh, mesh.surface(surface_id).faces().front().id(), submesh_id);
    }

    surface_ids = close_border(mesh, border_halfedges1, surface_name, -halfspace.plane().normal());
    for (id_type surface_id : surface_ids)
    {
        id_type submesh_id = mesh.get_submesh_id(mesh.surface(surface_id).submesh().name() + "_1");
        set_submesh_connected(mesh, mesh.surface(surface_id).faces().front().id(), submesh_id);
    }

    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::brep::split(Submesh<Traits>& submesh, const geometry::HalfSpace<typename Traits::vector_traits>& halfspace, const std::string& surface_name)
{
    auto& mesh = submesh.mesh();

    submesh.reset_all(); // for_each_vertex_position also does this ...

    // Mark exterior vertices
    // Vertices in halfspace border plane are considered interior

    bool exterior = false;
    for_each_vertex_position(submesh,
        [&exterior, &halfspace](const auto& vertex) -> bool
        {
            exterior = halfspace.exterior(vertex.attributes().position());
            return true;
        },
        [&exterior](auto& vertex)
        {
            vertex.set_marked(exterior);
            return;
        }
    );

    // Split intersecting faces

    auto face_ids = submesh.face_ids();
    for (id_type face_id : face_ids)
    {
        auto& face = mesh.face(face_id);

        bool all_interior = true;
        bool all_exterior = true;
        for (const auto& halfedge : face.halfedges())
        {
            if (halfedge.vertex().marked())
            {
                all_interior = false;
            }
            else
            {
                all_exterior = false;
            }
        }

        if (all_interior)
        {
            continue;
        }
        if (all_exterior)
        {
            face.set_marked();
            continue;
        }

        id_type halfedge_id_transition_out = nullid;
        id_type halfedge_id_transition_in = nullid;

        for (const auto& halfedge : face.halfedges())
        {
            bool marked0 = halfedge.vertex().marked();
            bool marked1 = halfedge.next().vertex().marked();

            if (marked0 && !marked1)
            {
                halfedge_id_transition_in = halfedge.id();
            }
            else if (!marked0 && marked1)
            {
                halfedge_id_transition_out = halfedge.id();
            }
        }

        assert(halfedge_id_transition_out != nullid);
        assert(halfedge_id_transition_in != nullid);

        // Don't split if non-exterior point already intersects halfspace plane
        if (!halfspace.boundary(mesh.halfedge(halfedge_id_transition_in).next().vertex_attributes().position()))
        {
            auto line = to_line(mesh.halfedge(halfedge_id_transition_in));
            auto intersection = geometry::intersection(line, halfspace.plane());
            assert(intersection.locus() == geometry::Locus::Point);
            auto position = intersection.point();
            split_edge(mesh, halfedge_id_transition_in, position);
        }
        else if (mesh.halfedge(halfedge_id_transition_in).next().id() == halfedge_id_transition_out)
        {
            // The only halfspace plane intersection is this vertex, so this is actually an exterior face, mark it as such
            mesh.halfedge(halfedge_id_transition_out).vertex().set_marked();
            face.set_marked();
            continue;
        }
        else if (halfspace.boundary(mesh.halfedge(halfedge_id_transition_out).vertex_attributes().position()))
        {
            // Both inward and outward transitioning halfedges intersect with the halfspace plane
            // If all vertices between halfedge_id_transition_in and halfedge_id_transition_out are in halfspace plane or exterior, then this is an exterior face

            exterior = true;
            for (id_type halfedge_id = mesh.halfedge(halfedge_id_transition_in).next().id(); halfedge_id != halfedge_id_transition_out; halfedge_id = mesh.halfedge(halfedge_id).next_id())
            {
                bool incident = halfspace.boundary(mesh.halfedge(halfedge_id).vertex_attributes().position());
                if (!mesh.halfedge(halfedge_id).marked() && !incident)
                {
                    exterior = false;
                    break;
                }
            }

            if (exterior)
            {
                for (id_type halfedge_id = mesh.halfedge(halfedge_id_transition_in).next().id(); halfedge_id != halfedge_id_transition_out; halfedge_id = mesh.halfedge(halfedge_id).next_id())
                {
                    mesh.halfedge(halfedge_id).vertex().set_marked();
                }

                mesh.halfedge(halfedge_id_transition_out).vertex().set_marked();
                face.set_marked();
                continue;
            }
        }

        halfedge_id_transition_in = mesh.halfedge(halfedge_id_transition_in).next_id(); // Adjust so vertex is on halfspace plane
        assert(halfspace.boundary(mesh.halfedge(halfedge_id_transition_in).vertex_attributes().position()));

        assert(!mesh.halfedge(halfedge_id_transition_in).vertex().marked());
        assert(!mesh.halfedge(halfedge_id_transition_out).vertex().marked());

        if (!halfspace.boundary(mesh.halfedge(halfedge_id_transition_out).vertex_attributes().position()))
        {
            auto line = to_line(mesh.halfedge(halfedge_id_transition_out));
            auto intersection = geometry::intersection(line, halfspace.plane());
            assert(intersection.locus() == geometry::Locus::Point);
            auto position = intersection.point();
            split_edge(mesh, halfedge_id_transition_out, position);

            halfedge_id_transition_out = mesh.halfedge(halfedge_id_transition_out).next_id(); // Adjust so vertex is on halfspace plane
        }

        mesh.halfedge(halfedge_id_transition_out).vertex().set_marked();

        assert(halfspace.boundary(mesh.halfedge(halfedge_id_transition_out).vertex_attributes().position()));

        // Split face into interior and exterior faces
        split_face(mesh, halfedge_id_transition_in, halfedge_id_transition_out);
        mesh.halfedge(halfedge_id_transition_out).prev().vertex().set_marked();
        mesh.halfedge(halfedge_id_transition_out).face().set_marked();

        // ** instead of marking here, split, uncouple, and add directly to border_halfedges ...
    }

    // Delete all vertices, edges, and faces that are exterior to the halfspace plane, store remaining border halfedges

    // border_halfedges[halfedge_id] = submesh_id
    BorderHalfedges border_halfedges0;
    BorderHalfedges border_halfedges1;

    face_ids = submesh.face_ids();
    for (id_type face_id : face_ids)
    {
        auto& face = mesh.face(face_id);

        if (face.marked())
        {
            for (auto& halfedge : face.halfedges())
            {
                assert(halfedge.vertex().marked());
            }

            continue;
        }

        for (auto& halfedge : face.halfedges())
        {
            assert(!halfedge.vertex().marked());
            assert(!halfedge.border());

            if (halfedge.partner().vertex().marked())
            {
                border_halfedges0.insert({halfedge.id(), face.submesh_id()});
                border_halfedges1.insert({halfedge.partner_id(), face.submesh_id()});
                halfedge.partner().set_border();
                halfedge.set_border();
            }
        }
    }

    assert(border_halfedges0.size() == border_halfedges1.size());
    if (border_halfedges0.empty()) // No intersection
    {
        return;
    }

    // Close off borders and assign each to a new submesh
    // This will still potentially create disconnected submeshes with one section per surface, need better naming ...

    auto surface_ids = close_border(mesh, border_halfedges0, surface_name, halfspace.plane().normal());
    for (id_type surface_id : surface_ids)
    {
        id_type submesh_id = mesh.get_submesh_id(mesh.surface(surface_id).submesh().name() + "_0");
        set_submesh_connected(mesh, mesh.surface(surface_id).faces().front().id(), submesh_id);
    }

    surface_ids = close_border(mesh, border_halfedges1, surface_name, -halfspace.plane().normal());
    for (id_type surface_id : surface_ids)
    {
        id_type submesh_id = mesh.get_submesh_id(mesh.surface(surface_id).submesh().name() + "_1");
        set_submesh_connected(mesh, mesh.surface(surface_id).faces().front().id(), submesh_id);
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::set_submesh_connected(M& mesh, id_type face_id, id_type submesh_id)
{
    mesh.reset_all(); // reset_faces would work here ...

    std::queue<id_type> face_ids({face_id});
    while (!face_ids.empty())
    {
        auto& f = mesh.face(face_ids.front());
        face_ids.pop();

        if (f.marked())
        {
            continue;
        }

        if (f.submesh_id() != submesh_id)
        {
            std::string surface_name = f.surface().name();

            id_type surface_id = mesh.get_surface_id(submesh_id, surface_name, f.surface().attributes());
            mesh.move_face(f.id(), surface_id);
        }

        f.set_marked();
        
        for (auto& halfedge : f.halfedges())
        {
            if (!halfedge.partner().face().marked())
            {
                face_ids.push(halfedge.partner().face_id());
            }
        }
    }

    return;
}

#endif // QUETZAL_BREP_SPLIT_HPP
