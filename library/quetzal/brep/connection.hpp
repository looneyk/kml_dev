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
#include <optional>
#include <ranges>
#include <string>

namespace quetzal::brep
{

    // Connect bodies A and B at coincident matching surfaces
    // Remove the original surfaces and their faces
    template<typename M>
    void weld(M& mesh, const std::string& surface_name_a, const std::string& surface_name_b);

    // Connect bodies A and B at coincident matching (partner) faces
    // Corresponding face halfedge partners will form new pairs
    // Remove the adjoining faces
    template<typename M>
    void weld(M& mesh, id_type face_id_a, id_type face_id_b);

    // Currently only supports surfaces that consist of a single face
    template<typename M>
    void attach(M& mesh, const std::string& surface_name_a, const std::string& surface_name_b, bool smooth = true);

    // Connect bodies A and B at coplanar faces using vertices and halfedges of faces A and B
    // Face B must fit completely within face A
    // Face A may not have holes
    // No new halfedges or vertices are added, only faces if there are any holes in B
    // Works by repurposing base B halfedges to form hole in face A and hole halfedges in base B to form new faces
    template<typename M>
    void attach(M& mesh, id_type face_id_a, id_type face_id_b, bool smooth = true);

    // better place for the following ...

    // FH is Face or Hole
    template<typename FH>
    id_type find_closest_vertex_halfedge(const FH& fh, id_type halfedge_id);

    // Create new faces between each edge of the face and the closest edge of its singular hole
    template<typename M>
    void connect_non_planar(M& mesh, id_type face_id_a, bool smooth);

} // namespace quetzal::brep

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::weld(M& mesh, const std::string& surface_name_a, const std::string& surface_name_b)
{
    auto& surface_a = mesh.surface(surface_name_a);
    auto& surface_b = mesh.surface(surface_name_b);
    assert(surface_a.face_count() == surface_b.face_count());

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

    auto faces_index_a = sort_faces(surface_a);
    auto faces_index_b = sort_faces(surface_b);
    assert(faces_index_a.size() == faces_index_b.size());

    for (const auto& [ia, ib] : std::views::zip(faces_index_a, faces_index_b))
    {
//        assert(float_eq(ia->first, ib->first));
//auto a = ia->first;
//auto b = ib->first;
assert(float_eq(ia.first, ib.first, 1024)); // imprecise centroid calculation? ...
        weld(mesh, ia.second, ib.second);
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

    for (auto& hole_a : face_a.holes())
    {
        id_type b_hole_halfedge_id = find_matching_hole_halfedge(face_b, hole_a);
        assert(mesh.valid_halfedge_id(b_hole_halfedge_id));

//        assert(hole_a.halfedge_count() == holeb.halfedge_count());

        for (auto& halfedge_a : hole_a.halfedges())
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
void quetzal::brep::attach(M& mesh, const std::string& surface_name_a, const std::string& surface_name_b, bool smooth)
{
    auto& surface_a = mesh.surface(surface_name_a);
    auto& surface_b = mesh.surface(surface_name_b);

    // so far this only handles the single face per surface case ...
    assert(surface_a.face_count() == surface_b.face_count());
    assert(surface_a.face_count() == 1);

    attach(mesh, surface_a.faces().front().id(), surface_b.faces().front().id(), smooth);
    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::attach(M& mesh, id_type face_id_a, id_type face_id_b, bool smooth)
{
    [[maybe_unused]] id_type halfedge_id_a = mesh.face(face_id_a).halfedge_id();
    id_type halfedge_id_b = mesh.face(face_id_b).halfedge_id();
    assert(mesh.valid_halfedge_id(halfedge_id_a));
    assert(mesh.valid_halfedge_id(halfedge_id_b));

    auto& face_a = mesh.face(face_id_a);
    auto& face_b = mesh.face(face_id_b);

// start out by moving all submesh_b surfaces to submesh_a ...

    id_type submesh_id = face_a.submesh_id();
    id_type surface_id0 = face_a.surface_id();
    auto surface_name = face_a.surface().name();
    auto as = face_a.surface().attributes();
    auto af = face_a.attributes();
    auto normal0 = af.normal();
    size_t n = face_a.halfedge_count();

    // Built before face A is modified below, so the original face's texcoords are applied
    std::optional<geometry::PolygonMapping<typename M::vertex_attributes_type>> mapping_a;
    if constexpr (geometry::Textured<typename M::vertex_attributes_type>)
    {
        mapping_a.emplace(face_texcoord_corners(face_a), normal0);
    }

    // Fix face id and apply face A normal to transferred vertices
    for (auto& halfedge : face_b.halfedges())
    {
        halfedge.set_face_id(face_id_a);
        halfedge.vertex_attributes().set_normal(normal0);
    }

    face_a.create_hole(halfedge_id_b);

    if constexpr (geometry::Textured<typename M::vertex_attributes_type>)
    {
        interpolate_texcoords(mapping_a.value(), face_b);
    }

    size_t i = n;
    auto hole_ids = face_b.hole_ids();
    for (id_type hole_id : hole_ids)
    {
        id_type surface_id = smooth ? surface_id0 : mesh.create_surface(submesh_id, format("{}_{}", surface_name, i), as);
        id_type face_id = mesh.create_face(surface_id, hole_id, af);
        auto& face = mesh.face(face_id);

        for (auto& halfedge : face.halfedges())
        {
            halfedge.set_face_id(face_id);
            halfedge.vertex_attributes().set_normal(normal0);
        }

        if constexpr (geometry::Textured<typename M::vertex_attributes_type>)
        {
            if (smooth)
            {
                interpolate_texcoords(mapping_a.value(), face);
            }
            else
            {
                calculate_face_texcoords(mesh, face.id());
            }
        }

        ++i;
    }

    // all of face_b's halfedges and vertices have been repurposed, so just bookkeeping here
    mesh.unlink_face(face_id_b);
    mesh.face(face_id_b).set_deleted();
    return;
}

//------------------------------------------------------------------------------
template<typename FH>
quetzal::id_type quetzal::brep::find_closest_vertex_halfedge(const FH& fh, id_type halfedge_id)
{
    auto position = fh.mesh().halfedge(halfedge_id).vertex_attributes().position();

    auto d_min = std::numeric_limits<typename FH::mesh_type::value_type>::max();
    id_type id_min = nullid;

    for (const auto& halfedge : fh.halfedges())
    {
        auto d = (halfedge.vertex_attributes().position() - position).norm_squared();
        if (d < d_min)
        {
            d_min = d;
            id_min = halfedge.id();
        }
    }

    return id_min;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::connect_non_planar(M& mesh, id_type face_id_a, bool smooth)
{
    const auto& face_a = mesh.face(face_id_a);

    assert(face_a.hole_count() == 1);
    const auto& hole = face_a.holes().front();

    id_type submesh_id = face_a.submesh_id();
    id_type surface_id0 = face_a.surface_id();
    auto surface_name = face_a.surface().name();
    auto as = face_a.surface().attributes();
    size_t n = face_a.halfedge_count();

    id_type id_a = face_a.halfedge_id();
    id_type id_b = find_closest_vertex_halfedge(hole, id_a);

    size_t nf = mesh.face_store_count();
    id_type face_id_prev = nf + n - 1;

    for (size_t i = 0; i < n; ++i)
    {
        id_type surface_id = smooth ? surface_id0 : mesh.create_surface(submesh_id, std::format("{}_{}", surface_name, i), as);
        id_type face_id = mesh.create_face(surface_id, id_a, {});

        mesh.halfedge(id_a).set_face_id(face_id);
        mesh.halfedge(id_b).set_face_id(face_id_prev);

        id_type id_a_next = mesh.halfedge(id_a).next_id();
        id_type id_b_next = mesh.halfedge(id_b).prev_id();

        connect(mesh, id_a, id_b);
        face_id_prev = face_id;

        id_a = id_a_next;
        id_b = id_b_next;
    }

    for (size_t i = 0; i < n; ++i)
    {
        auto& face = mesh.face(nf + i);
        auto normal = geometry::normal(face);
        face.attributes().set_normal(normal);

        if (!smooth)
        {
            face.surface().attributes().set_normal(normal); // arbitrary, discard? no, this is that one face per surface case, in which case there is no need for surface normal anyway ...

            if constexpr (geometry::Textured<typename M::vertex_attributes_type>)
            {
                calculate_surface_texcoords(mesh, face.surface_id());
            }
        }
    }

    // all of face_a's halfedges and vertices have been repurposed
    mesh.unlink_face(face_id_a);
    mesh.face(face_id_a).set_deleted();

    return;
}

#endif // QUETZAL_BREP_CONNECTION_HPP
