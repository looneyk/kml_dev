#if !defined(QUETZAL_BREP_BORDER_HPP)
#define QUETZAL_BREP_BORDER_HPP
//------------------------------------------------------------------------------
// brep
// border.hpp
//------------------------------------------------------------------------------

#include "BorderFragment.hpp"
#include "geometry.hpp"
#include "quetzal/common/id.hpp"
#include "quetzal/geometry/Polygon.hpp"
#include "quetzal/geometry/Projection.hpp"
#include <functional>

namespace quetzal::brep
{

    // Returns the next border halfedge id, same halfedge ordering as a surface border
    // The halfedge coming in does not itself need to be a border
    template<typename M>
    id_type next_border_halfedge_id(const M& mesh, id_type halfedge_id);

    // Returns the prev border halfedge id, same halfedge ordering as a surface border
    // The halfedge coming in does not itself need to be a border
    template<typename M>
    id_type prev_border_halfedge_id(const M& mesh, id_type halfedge_id);

    // Returns the id of the next halfedge in order around the surface
    template<typename M>
    id_type next_surface_halfedge_id(const M& mesh, id_type halfedge_id);

    // Returns the id of the previous halfedge in order around the surface
    template<typename M>
    id_type prev_surface_halfedge_id(const M& mesh, id_type halfedge_id);

    // Returns the id of the next halfedge in order around the surface with the same adjacent surface
    template<typename M>
    id_type next_seam_halfedge_id(const M& mesh, id_type halfedge_id);

    // Returns the id of the previous halfedge in order around the surface with the same adjacent surface
    template<typename M>
    id_type prev_seam_halfedge_id(const M& mesh, id_type halfedge_id);

    template<typename M>
    size_t border_edge_count(const M& mesh, id_type halfedge_id);

    // Halfedges in the order for a face closing off the open border area
    template<typename M>
    std::vector<id_type> border_halfedges(const M& mesh, id_type halfedge_id);

    // Create a polygon from the border vertices, in the order for a face closing off the open border area
    template<typename M>
    geometry::Polygon<typename M::vector_traits> to_border_polygon(const M& mesh, id_type halfedge_id);

    // Create a 2D polygon to use for face/hole association
    template<typename M>
    auto to_border_polygon(const M& mesh, id_type halfedge_id, const geometry::Projection<typename M::vector_traits>& projection) -> geometry::Polygon<typename geometry::Projection<typename M::vector_traits>::projected_traits>;

    // Delete all components connected to this at any level
    template<typename M>
    void delete_border_fragment(M& mesh, id_type halfedge_id, std::function<bool(const typename M::halfedge_type&)> border_condition = [](const typename M::halfedge_type& halfedge) -> bool { return halfedge.border(); });

    template<typename M>
    void connect_border_fragments(M& mesh, id_type halfedge_id_a, id_type halfedge_id_b);

    template<typename M>
    void set_border_fragment_submesh(M& mesh, id_type halfedge_id, const std::string& name);

    template<typename M>
    void set_border_fragment_submesh(M& mesh, id_type halfedge_id, id_type submesh_id);

    // Handles a single face and connected faces recursively
    template<typename M>
    void set_border_fragment_submesh_internal(M& mesh, id_type halfedge_id, id_type submesh_id);

    template<typename M>
    void dump_border_fragment(M& mesh, id_type halfedge_id);

    // Handles a single face and connected faces recursively
    template<typename M>
    void dump_border_fragment_internal(M& mesh, id_type halfedge_id);

} // namespace quetzal::brep

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::brep::next_border_halfedge_id(const M& mesh, id_type halfedge_id)
{
    const auto& halfedge = mesh.halfedge(halfedge_id);

    for (id_type id = halfedge.next_id(); ; id = mesh.halfedge(id).partner().next_id())
    {
        if (mesh.halfedge(id).border())
        {
            return id;
        }

        if (mesh.halfedge(id).partner_id() == halfedge_id)
        {
            // This should normally never happen, a vertex cannot have an odd number of border halfedges
            // During construction however, this can occur as coupling or uncoupling is being performed
            break;
        }
    }

    return nullid;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::brep::prev_border_halfedge_id(const M& mesh, id_type halfedge_id)
{
    const auto& halfedge = mesh.halfedge(halfedge_id);

    for (id_type id = halfedge.prev_id(); ; id = mesh.halfedge(id).partner().prev_id())
    {
        if (mesh.halfedge(id).border())
        {
            return id;
        }

        if (mesh.halfedge(id).partner_id() == halfedge_id)
        {
            // This should normally never happen, a vertex cannot have an odd number of border halfedges
            // During construction however, this can occur as coupling or uncoupling is being performed
            break;
        }
    }

    return nullid;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::brep::next_surface_halfedge_id(const M& mesh, id_type halfedge_id)
{
    const auto& halfedge = mesh.halfedge(halfedge_id);
    assert(halfedge.surface_border());

    id_type halfedge_id_next = halfedge.next_id();
    while (!mesh.halfedge(halfedge_id_next).surface_border())
    {
        halfedge_id_next = mesh.halfedge(halfedge_id_next).partner().next_id();
    }

    return halfedge_id_next;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::brep::prev_surface_halfedge_id(const M& mesh, id_type halfedge_id)
{
    const auto& halfedge = mesh.halfedge(halfedge_id);
    assert(halfedge.surface_border());

    id_type halfedge_id_prev = halfedge.prev_id();
    while (!mesh.halfedge(halfedge_id_prev).surface_border())
    {
        halfedge_id_prev = mesh.halfedge(halfedge_id_prev).partner().prev_id();
    }

    return halfedge_id_prev;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::brep::next_seam_halfedge_id(const M& mesh, id_type halfedge_id)
{
    id_type halfedge_id_next = next_surface_halfedge_id(mesh, halfedge_id);
    id_type idSurface = mesh.halfedge(halfedge_id).surface_id();
    return mesh.halfedge(halfedge_id_next).partner_surface_id() == idSurface ? halfedge_id_next : nullid;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::brep::prev_seam_halfedge_id(const M& mesh, id_type halfedge_id)
{
    id_type halfedge_id_prev = prev_surface_halfedge_id(mesh, halfedge_id);
    id_type idSurface = mesh.halfedge(halfedge_id).surface_id();
    return mesh.halfedge(halfedge_id_prev).partner_surface_id() == idSurface ? halfedge_id_prev : nullid;
}

//------------------------------------------------------------------------------
template<typename M>
size_t quetzal::brep::border_edge_count(const M& mesh, id_type halfedge_id)
{
    assert(mesh.valid_halfedge_id(halfedge_id));
    assert(mesh.halfedge(halfedge_id).border());

    size_t n = 0;

    id_type id = halfedge_id;
    do
    {
        ++n;
        id = next_border_halfedge_id(mesh, id);
    } while (id != halfedge_id);

    return n;
}

//------------------------------------------------------------------------------
template<typename M>
std::vector<quetzal::id_type> quetzal::brep::border_halfedges(const M& mesh, id_type halfedge_id)
{
    assert(mesh.valid_halfedge_id(halfedge_id));

    std::vector<id_type> ids;

    id_type id = halfedge_id;
    do
    {
        assert(mesh.halfedge(id).border());
        ids.push_back(id);
        id = prev_border_halfedge_id(mesh, id);
    } while (id != halfedge_id);

    return ids;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::geometry::Polygon<typename M::vector_traits> quetzal::brep::to_border_polygon(const M& mesh, id_type halfedge_id)
{
    assert(mesh.valid_halfedge_id(halfedge_id));
    assert(mesh.halfedge(halfedge_id).border());

    geometry::Polygon<typename M::vector_traits> polygon;

    id_type id = halfedge_id;
    do
    {
        polygon.vertices().push_back(mesh.halfedge(id).vertex_attributes().position());
        id = prev_border_halfedge_id(mesh, id);
    } while (id != halfedge_id);

    return polygon;
}

//------------------------------------------------------------------------------
template<typename M>
auto quetzal::brep::to_border_polygon(const M& mesh, id_type halfedge_id, const geometry::Projection<typename M::vector_traits>& projection) -> geometry::Polygon<typename geometry::Projection<typename M::vector_traits>::projected_traits>
{
    assert(mesh.valid_halfedge_id(halfedge_id));
    assert(mesh.halfedge(halfedge_id).border());

    geometry::Polygon<typename geometry::Projection<typename M::vector_traits>::projected_traits> polygon;

    id_type id = halfedge_id;
    do
    {
        polygon.vertices().push_back(projection(mesh.halfedge(id).vertex_attributes().position()));
        id = prev_border_halfedge_id(mesh, id);
    } while (id != halfedge_id);

    return polygon;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::delete_border_fragment(M& mesh, id_type halfedge_id, std::function<bool(const typename M::halfedge_type&)> border_condition)
{
    // Check that this fragment has not already been deleted by an earlier connection
    if (mesh.halfedge_store()[halfedge_id].deleted())
    {
        return;
    }

    id_type face_id = mesh.halfedge_store()[halfedge_id].face_id();
    auto& face = mesh.face_store()[face_id];

    // Cache connected face halfedge ids before deleting the face
    std::set<id_type> halfedge_ids;
    for (const auto& halfedge : face.halfedges())
    {
        assert(!halfedge.deleted());
        if (!halfedge.border())
        {
            halfedge_ids.insert(halfedge.partner_id());
        }
    }

    mesh.delete_face(face_id);

    // Delete connected faces recursively, checking that the existence and deletion conditions still hold
    for (const auto& id : halfedge_ids)
    {
        const auto& halfedge = mesh.halfedge_store()[id];
        if (!halfedge.deleted())
        {
            delete_border_fragment(mesh, id, border_condition);
        }
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::connect_border_fragments(M& mesh, id_type halfedge_id_a, id_type halfedge_id_b)
{
    id_type id_a = halfedge_id_a;
    id_type id_b = halfedge_id_b;

    assert(vertex_position(mesh.halfedge(id_a).next()) == vertex_position(mesh.halfedge(next_border_halfedge_id(mesh, id_a))));
    assert(vertex_position(mesh.halfedge(id_b).next()) == vertex_position(mesh.halfedge(next_border_halfedge_id(mesh, id_b))));

    while (!float_eq(vertex_position(mesh.halfedge(id_a)), vertex_position(mesh.halfedge(id_b).next())))
    {
        id_b = next_border_halfedge_id(mesh, id_b);
    }
    assert(vertex_position(mesh.halfedge(id_a)) == vertex_position(mesh.halfedge(id_b).next()));
    assert(vertex_position(mesh.halfedge(id_b)) == vertex_position(mesh.halfedge(id_a).next()));

    for (;;)
    {
        id_type id_a_next = next_border_halfedge_id(mesh, id_a);
        id_type id_b_next = prev_border_halfedge_id(mesh, id_b);
        if (id_a_next == nullid || id_b_next == nullid)
        {
            assert(id_a_next == nullid && id_b_next == nullid);
            return;
        }

        assert(vertex_position(mesh.halfedge(id_a)) == vertex_position(mesh.halfedge(id_b).next()));
        assert(vertex_position(mesh.halfedge(id_b)) == vertex_position(mesh.halfedge(id_a).next()));

        mesh.halfedge(id_a).set_partner_id(id_b);
        mesh.halfedge(id_b).set_partner_id(id_a);

        id_a = id_a_next;
        id_b = id_b_next;
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::set_border_fragment_submesh(M& mesh, id_type halfedge_id, const std::string& name)
{
    id_type submesh_id = mesh.contains_submesh(name) ? mesh.submesh_id(name) : mesh.create_submesh(name);
    set_border_fragment_submesh(mesh, halfedge_id, submesh_id);
    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::set_border_fragment_submesh(M& mesh, id_type halfedge_id, id_type submesh_id)
{
    mesh.reset_all(); // border fragment specific reset_all ...
    set_border_fragment_submesh_internal(mesh, halfedge_id, submesh_id);
    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::set_border_fragment_submesh_internal(M& mesh, id_type halfedge_id, id_type submesh_id)
{
    id_type face_id = mesh.halfedge(halfedge_id).face_id();
    auto& face = mesh.face(face_id);
    // Surface name disambiguation must be done by caller
    //auto surface_name = std::format("{}_{}", face.submesh().name(), face.surface().name());
    auto surface_name = face.surface().name();
    mesh.unlink_face(face_id);
    mesh.add_surface_face(submesh_id, surface_name, face_id);
    face.set_visited();

    for (const auto& halfedge : face.halfedges())
    {
        id_type partner_id = halfedge.partner_id();
        if (partner_id == nullid || mesh.halfedge(partner_id).face().visited())
        {
            continue;
        }

        set_border_fragment_submesh_internal(mesh, partner_id, submesh_id);
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::dump_border_fragment(M& mesh, id_type halfedge_id)
{
    mesh.reset_all(); // border fragment specific reset_all ...
    dump_border_fragment_internal(mesh, halfedge_id);
    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::dump_border_fragment_internal(M& mesh, id_type halfedge_id)
{
    id_type face_id = mesh.halfedge(halfedge_id).face_id();
    dump_face(mesh, face_id);

    const auto& face = mesh.face(face_id);
    face.set_visited();

    for (const auto& halfedge : face.halfedges())
    {
        id_type partner_id = halfedge.partner_id();
        if (partner_id == nullid || mesh.halfedge(partner_id).face().visited())
        {
            continue;
        }

        dump_border_fragment_internal(mesh, partner_id);
    }
}

#endif // QUETZAL_BREP_BORDER_HPP
