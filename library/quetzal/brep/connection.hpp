#if !defined(QUETZAL_BREP_CONNECTION_HPP)
#define QUETZAL_BREP_CONNECTION_HPP
//------------------------------------------------------------------------------
// brep
// connection.hpp
//------------------------------------------------------------------------------

#include "geometry.hpp"
#include "mesh_util.hpp"
#include "texcoord.hpp"
#include <format>
#include <limits>
#include <map>
#include <ranges>
#include <string>

namespace quetzal::brep
{

    // Connect bodies A and B at coincident matching surfaces
    // Remove the original surfaces and their faces
    template<typename M>
    void weld(M& mesh, const std::string& nameSurfaceA, const std::string& nameSurfaceB);

    // Connect bodies A and B at coincident matching (partner) faces
    // Corresponding face halfedge partners will form new pairs
    // Remove the adjoining faces
    template<typename M>
    void weld(M& mesh, id_type idFaceA, id_type idFaceB);

    // Currently only supports surfaces that consist of a single face
    template<typename M>
    void attach(M& mesh, const std::string& nameSurfaceA, const std::string& nameSurfaceB, bool bSmooth = true);

    // Connect bodies A and B at coplanar faces using vertices and halfedges of faces A and B
    // Face B must fit completely within face A
    // Face A may not have holes
    // No new halfedges or vertices are added, only faces if there are any holes in B
    // Works by repurposing base B halfedges to form hole in face A and hole halfedges in base B to form new faces
    template<typename M>
    void attach(M& mesh, id_type idFaceA, id_type idFaceB, bool bSmooth = true);

    // better place for the following ...

    // FH is Face or Hole
    template<typename FH>
    id_type find_closest_vertex_halfedge(const FH& fh, id_type idHalfedge);

    // Create new faces between each edge of the face and the closest edge of its singular hole
    template<typename M>
    void connect_non_planar(M& mesh, id_type idFaceA, bool bSmooth);

} // namespace quetzal::brep

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::weld(M& mesh, const std::string& nameSurfaceA, const std::string& nameSurfaceB)
{
    auto& surfaceA = mesh.surface(nameSurfaceA);
    auto& surfaceB = mesh.surface(nameSurfaceB);
    assert(surfaceA.face_count() == surfaceB.face_count());

    using index_type = std::map<typename M::point_type, id_type, math::PriorityLess<typename M::point_type>>;

    auto sort_faces = [&mesh](const typename M::surface_type& surface) -> index_type
    { 
        index_type index;
        for (const auto& face : surface.faces())
        {
            index[vertex_centroid(face)] = face.id();
        }
        return index;
    };

    auto indexFacesA = sort_faces(surfaceA);
    auto indexFacesB = sort_faces(surfaceB);
    assert(indexFacesA.size() == indexFacesB.size());

    for (const auto& [iA, iB] : std::views::zip(indexFacesA, indexFacesB))
    {
//        assert(float_eq(iA->first, iB->first));
//auto a = iA->first;
//auto b = iB->first;
assert(float_eq(iA.first, iB.first, 1024)); // imprecise centroid calculation? ...
        weld(mesh, iA.second, iB.second);
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::weld(M& mesh, id_type face_id_a, id_type face_id_b)
{
    auto& face_a = mesh.face(face_id_a);
    auto& face_b = mesh.face(face_id_b);
 
    assert(face_a.halfedge_count() == face_b.halfedge_count());
    assert(face_a.hole_count() == face_b.hole_count());

    auto partner_ids = unify_vertex_positions(face_a, face_b);

    for (auto [halfedge_id_a, halfedge_id_b] : partner_ids)
    {
        auto& halfedge_a = mesh.halfedge(halfedge_id_a);
        auto& halfedge_b = mesh.halfedge(halfedge_id_b);

        // Fine even if A and B are partners
        halfedge_a.partner().set_partner_id(halfedge_b.partner_id());
        halfedge_b.partner().set_partner_id(halfedge_a.partner_id());

        halfedge_a.set_border();
        halfedge_b.set_border();
    }

    // Need to do the same for holes here, connect them through, holes will be deleted along with their faces below
    // Use the scheme above, unify_vertex_positions needs to work with combinations of face and hole ...

    for (auto& holeA : face_a.holes())
    {
        id_type b_hole_halfedge_id = find_matching_hole_halfedge(face_b, holeA);
        assert(mesh.valid_halfedge_id(b_hole_halfedge_id));

//        assert(holeA.halfedge_count() == holeB.halfedge_count());

        for (auto& halfedge_a : holeA.halfedges())
        {
            auto& halfedge_b = mesh.halfedge(b_hole_halfedge_id);

            assert(float_eq(halfedge_a.vertex_attributes().position(), halfedge_b.next().vertex_attributes().position()));
            assert(float_eq(halfedge_b.vertex_attributes().position(), halfedge_a.next().vertex_attributes().position()));

            // Ensure vertex positions are consistent
            set_vertex_position(halfedge_b.next().vertex(), halfedge_a.vertex_attributes().position());

            // Fine even if A and B are partners
            halfedge_a.partner().set_partner_id(halfedge_b.partner_id());
            halfedge_b.partner().set_partner_id(halfedge_a.partner_id());

            halfedge_a.set_border();
            halfedge_b.set_border();

            b_hole_halfedge_id = halfedge_b.prev_id();
        }
    }

    mesh.delete_face(face_id_a);
    mesh.delete_face(face_id_b);
    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::attach(M& mesh, const std::string& nameSurfaceA, const std::string& nameSurfaceB, bool bSmooth)
{
    auto& surfaceA = mesh.surface(nameSurfaceA);
    auto& surfaceB = mesh.surface(nameSurfaceB);

    // so far this only handles the single face per surface case ...
    assert(surfaceA.face_count() == surfaceB.face_count());
    assert(surfaceA.face_count() == 1);

    attach(mesh, surfaceA.faces().front().id(), surfaceB.faces().front().id(), bSmooth);
    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::attach(M& mesh, id_type idFaceA, id_type idFaceB, bool bSmooth)
{
    id_type idHalfedgeA = mesh.face(idFaceA).halfedge_id();
    id_type idHalfedgeB = mesh.face(idFaceB).halfedge_id();
    assert(mesh.valid_halfedge_id(idHalfedgeA));
    assert(mesh.valid_halfedge_id(idHalfedgeB));

    auto& faceA = mesh.face(idFaceA);
    auto& faceB = mesh.face(idFaceB);

// start out by moving all submeshB surfaces to submeshA ...

    id_type idSubmesh = faceA.submesh_id();
    id_type idSurface0 = faceA.surface_id();
    auto nameSurface = faceA.surface().name();
    auto as = faceA.surface().attributes();
    auto af = faceA.attributes();
    auto normal0 = af.normal();
    size_t n = faceA.halfedge_count();

    // Fix face id and apply face A normal to transferred vertices
    for (auto& halfedge : faceB.halfedges())
    {
        halfedge.set_face_id(idFaceA);
        halfedge.vertex_attributes().set_normal(normal0);
    }

    faceA.create_hole(idHalfedgeB);

    if constexpr (M::vertex_attributes_type::contains(geometry::AttributesFlags::Texcoord0))
    {
        interpolate_texcoords(faceA, faceB);
    }

    size_t i = n;
    auto hole_ids = faceB.hole_ids();
    for (id_type hole_id : hole_ids)
    {
        id_type idSurface = bSmooth ? idSurface0 : mesh.create_surface(idSubmesh, format("{}_{}", nameSurface, i), as);
        id_type idFace = mesh.create_face(idSurface, hole_id, af);
        auto& face = mesh.face(idFace);

        for (auto& halfedge : face.halfedges())
        {
            halfedge.set_face_id(idFace);
            halfedge.vertex_attributes().set_normal(normal0);
        }

        if constexpr (M::vertex_attributes_type::contains(geometry::AttributesFlags::Texcoord0))
        {
            if (bSmooth)
            {
                interpolate_texcoords(mesh.face(idFaceA), face);
            }
            else
            {
                calculate_face_texcoords(mesh, face.id());
            }
        }

        ++i;
    }

    // All of faceB's halfedges and vertices have been repurposed, so just bookkeeping here
    mesh.unlink_face(idFaceB);
    mesh.face(idFaceB).set_deleted();
    return;
}

//------------------------------------------------------------------------------
template<typename FH>
quetzal::id_type quetzal::brep::find_closest_vertex_halfedge(const FH& fh, id_type idHalfedge)
{
    auto position = fh.mesh().halfedge(idHalfedge).vertex_attributes().position();

    auto dMin = std::numeric_limits<typename FH::mesh_type::value_type>::max();
    id_type idMin = nullid;

    for (const auto& halfedge : fh.halfedges())
    {
        auto d = (halfedge.vertex_attributes().position() - position).norm_squared();
        if (d < dMin)
        {
            dMin = d;
            idMin = halfedge.id();
        }
    }

    return idMin;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::connect_non_planar(M& mesh, id_type idFaceA, bool bSmooth)
{
    const auto& faceA = mesh.face(idFaceA);

    assert(faceA.hole_count() == 1);
    const auto& hole = faceA.holes().front();

    id_type idSubmesh = faceA.submesh_id();
    id_type idSurface0 = faceA.surface_id();
    auto nameSurface = faceA.surface().name();
    auto as = faceA.surface().attributes();
    size_t n = faceA.halfedge_count();

    id_type idA = faceA.halfedge_id();
    id_type idB = find_closest_vertex_halfedge(hole, idA);

    size_t nf = mesh.face_store_count();
    id_type idFacePrev = nf + n - 1;

    for (size_t i = 0; i < n; ++i)
    {
        id_type idSurface = bSmooth ? idSurface0 : mesh.create_surface(idSubmesh, std::format("{}_{}", nameSurface, i), as);
        id_type idFace = mesh.create_face(idSurface, idA, {});

        mesh.halfedge(idA).set_face_id(idFace);
        mesh.halfedge(idB).set_face_id(idFacePrev);

        id_type idANext = mesh.halfedge(idA).next_id();
        id_type idBNext = mesh.halfedge(idB).prev_id();

        connect(mesh, idA, idB);
        idFacePrev = idFace;

        idA = idANext;
        idB = idBNext;
    }

    for (size_t i = 0; i < n; ++i)
    {
        auto& face = mesh.face(nf + i);
        auto normal = geometry::normal(face);
        face.attributes().set_normal(normal);

        if (!bSmooth)
        {
            face.surface().attributes().set_normal(normal); // arbitrary, discard? no, this is that one face per surface case, in which case there is no need for surface normal anyway ...

            if constexpr (M::vertex_attributes_type::contains(geometry::AttributesFlags::Texcoord0))
            {
                calculate_surface_texcoords(mesh, face.surface_id());
            }
        }
    }

    // All of faceA's halfedges and vertices have been repurposed
    mesh.unlink_face(idFaceA);
    mesh.face(idFaceA).set_deleted();

    return;
}

#endif // QUETZAL_BREP_CONNECTION_HPP
