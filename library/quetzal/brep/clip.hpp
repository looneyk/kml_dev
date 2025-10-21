#if !defined(QUETZAL_BREP_CLIP_HPP)
#define QUETZAL_BREP_CLIP_HPP
//------------------------------------------------------------------------------
// brep
// clip.hpp
//------------------------------------------------------------------------------

#include "geometry.hpp"
#include "mesh_util.hpp"
#include "quetzal/geometry/HalfSpace.hpp"
#include "quetzal/geometry/Line.hpp"
#include "quetzal/geometry/intersect.hpp"
#include <string>

namespace quetzal::brep
{

    // clip submesh in addition to or instead of mesh ...

    template<typename M>
    void clip(M& mesh, const geometry::HalfSpace<typename M::vector_traits>& halfspace, const std::string& nameSurface = "clip");

} // namespace quetzal::brep

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::clip(M& mesh, const geometry::HalfSpace<typename M::vector_traits>& halfspace, const std::string& nameSurface)
{
    mesh.reset_all();

    // Mark exterior vertices
    // Vertices in halfspace border plane are considered interior

    for (auto& vertex : mesh.vertices())
    {
        if (vertex.checked())
        {
            continue;
        }

        bool bExterior = halfspace.exterior(vertex.attributes().position());

        for (auto& halfedge : vertex.halfedges())
        {
            halfedge.vertex().set_marked(bExterior);
            halfedge.vertex().set_checked();
        }
    }

    // Split intersecting faces

    id_type nFacesOrig = mesh.face_store_count();
    for (size_t nFace = 0; nFace < nFacesOrig; ++nFace)
    {
        auto& face = mesh.face_store()[nFace];
        if (face.deleted())
        {
            continue;
        }

        bool bAllInterior = true;
        bool bAllExterior = true;
        for (const auto& halfedge : face.halfedges())
        {
            if (halfedge.vertex().marked())
            {
                bAllInterior = false;
            }
            else
            {
                bAllExterior = false;
            }
        }

        if (bAllInterior)
        {
            continue;
        }
        if (bAllExterior)
        {
            face.set_marked();
            continue;
        }

        id_type idHalfedgeTransitionOut = nullid;
        id_type idHalfedgeTransitionIn = nullid;

        for (const auto& halfedge : face.halfedges())
        {
            bool bMarked0 = halfedge.vertex().marked();
            bool bMarked1 = halfedge.next().vertex().marked();

            if (bMarked0 && !bMarked1)
            {
                idHalfedgeTransitionIn = halfedge.id();
            }
            else if (!bMarked0 && bMarked1)
            {
                idHalfedgeTransitionOut = halfedge.id();
            }
        }

        assert(idHalfedgeTransitionOut != nullid);
        assert(idHalfedgeTransitionIn != nullid);

        // Don't split if non-exterior point already intersects halfspace plane
        if (!halfspace.boundary(mesh.halfedge(idHalfedgeTransitionIn).next().vertex_attributes().position()))
        {
            auto line = to_line(mesh.halfedge(idHalfedgeTransitionIn));
            auto intersection = geometry::intersection(line, halfspace.plane());
            assert(intersection.locus() == geometry::Locus::Point);
            auto position = intersection.point();
            split_edge(mesh, idHalfedgeTransitionIn, position);
        }
        else if (mesh.halfedge(idHalfedgeTransitionIn).next().id() == idHalfedgeTransitionOut)
        {
            // The only halfspace plane intersection is this vertex, so this is actually an exterior face, mark it as such
            mesh.halfedge(idHalfedgeTransitionOut).vertex().set_marked();
            face.set_marked();
            continue;
        }
        else if (halfspace.boundary(mesh.halfedge(idHalfedgeTransitionOut).vertex_attributes().position()))
        {
            // Both inward and outward transitioning halfedges intersect with the halfspace plane
            // If all vertices between idHalfedgeTransitionIn and idHalfedgeTransitionOut are in halfspace plane or exterior, then this is an exterior face

            bool bExterior = true;
            for (id_type idHalfedge = mesh.halfedge(idHalfedgeTransitionIn).next().id(); idHalfedge != idHalfedgeTransitionOut; idHalfedge = mesh.halfedge(idHalfedge).next_id())
            {
                bool bIncident = halfspace.boundary(mesh.halfedge(idHalfedge).vertex_attributes().position());
                if (!mesh.halfedge(idHalfedge).marked() && !bIncident)
                {
                    bExterior = false;
                    break;
                }
            }

            if (bExterior)
            {
                for (id_type idHalfedge = mesh.halfedge(idHalfedgeTransitionIn).next().id(); idHalfedge != idHalfedgeTransitionOut; idHalfedge = mesh.halfedge(idHalfedge).next_id())
                {
                    mesh.halfedge(idHalfedge).vertex().set_marked();
                }

                mesh.halfedge(idHalfedgeTransitionOut).vertex().set_marked();
                face.set_marked();
                continue;
            }
        }

        idHalfedgeTransitionIn = mesh.halfedge(idHalfedgeTransitionIn).next_id(); // Adjust so vertex is on halfspace plane
        assert(halfspace.boundary(mesh.halfedge(idHalfedgeTransitionIn).vertex_attributes().position()));

        assert(!mesh.halfedge(idHalfedgeTransitionIn).vertex().marked());
        assert(!mesh.halfedge(idHalfedgeTransitionOut).vertex().marked());

        if (!halfspace.boundary(mesh.halfedge(idHalfedgeTransitionOut).vertex_attributes().position()))
        {
            auto line = to_line(mesh.halfedge(idHalfedgeTransitionOut));
            auto intersection = geometry::intersection(line, halfspace.plane());
            assert(intersection.locus() == geometry::Locus::Point);
            auto position = intersection.point();
            split_edge(mesh, idHalfedgeTransitionOut, position);

            idHalfedgeTransitionOut = mesh.halfedge(idHalfedgeTransitionOut).next_id(); // Adjust so vertex is on halfspace plane
        }

        mesh.halfedge(idHalfedgeTransitionOut).vertex().set_marked();

        assert(halfspace.boundary(mesh.halfedge(idHalfedgeTransitionOut).vertex_attributes().position()));

        // Split face into interior and exterior faces
        split_face(mesh, idHalfedgeTransitionIn, idHalfedgeTransitionOut);
        mesh.halfedge(idHalfedgeTransitionOut).prev().vertex().set_marked();
        mesh.halfedge(idHalfedgeTransitionOut).face().set_marked();
    }

    // Delete all vertices, edges, and faces that are exterior to the halfspace plane, store remaining border halfedges

    // borderHalfedges[idHalfedge] = idSubmesh
    BorderHalfedges borderHalfedges;

    bool bInterior = true;
    for (auto& face : mesh.faces())
    {
        if (face.marked() == bInterior)
        {
            for (auto& halfedge : face.halfedges())
            {
                assert(halfedge.vertex().marked());
            }

            mesh.delete_face(face.id());
            continue;
        }

        for (auto& halfedge : face.halfedges())
        {
            assert(halfedge.vertex().marked() != bInterior);

            if (halfedge.border() || halfedge.partner().vertex().marked() == bInterior)
            {
                borderHalfedges.insert({halfedge.id(), face.submesh_id()});
                halfedge.set_border();
            }
        }
    }

    if (borderHalfedges.empty()) // No intersection
    {
        return;
    }

    close_border(mesh, borderHalfedges, nameSurface, bInterior ? halfspace.plane().normal() : -halfspace.plane().normal());
    return;
}

#endif // QUETZAL_BREP_CLIP_HPP
