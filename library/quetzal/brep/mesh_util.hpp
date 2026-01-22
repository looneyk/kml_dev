#if !defined(QUETZAL_BREP_MESH_UTIL_HPP)
#define QUETZAL_BREP_MESH_UTIL_HPP
//------------------------------------------------------------------------------
// brep
// mesh_util.hpp
//------------------------------------------------------------------------------

#include "Face.hpp"
#include "border.hpp"
#include "geometry.hpp"
#include "texcoord.hpp"
#include "quetzal/geometry/Attributes.hpp"
#include "quetzal/geometry/Point.hpp"
#include "quetzal/geometry/Polygon.hpp"
#include "quetzal/geometry/Projection.hpp"
#include "quetzal/math/floating_point.hpp"
#include "quetzal/math/math_util.hpp"
#include <array>
#include <functional>
#include <map>
#include <span>
#include <string>

//#include "validation.hpp" // ...
#include "quetzal/math/floating_point.hpp"
#include <iostream>

namespace quetzal::brep
{

    // Works with Surface, Submesh, and Mesh
    // could take renamer (defaulted) instead ...
    template<typename U>
    void add_face_surfaces(U& u, const std::string& prefix = "");

    template<typename M>
    void remove_degenerate_face(M& mesh, id_type face_id);

    template<typename M>
    void remove_degenerate_faces(M& mesh);

    // Create single face from all faces that contain this vertex, return the id of the resulting face
    template<typename M>
    id_type remove_vertex(M& mesh, id_type vertex_id);

    // Create single face from the two faces adjacent to this edge, return the id of the resulting face
    template<typename M>
    id_type remove_edge(M& mesh, id_type halfedge_id);

    template<typename M>
    void update_halfedge(typename M::halfedge_type& halfedge, M& mesh, id_type halfedge_id_offset, id_type vertex_id_offset, id_type face_id_offset);

    template<typename M>
    void update_vertex(typename M::vertex_type& vertex, M& mesh, id_type halfedge_id_offset, id_type vertex_id_offset);

    template<typename M>
    void update_face(typename M::face_type& face, M& mesh, id_type halfedge_id_offset, id_type face_id_offset, id_type surface_id, id_type submesh_id);

//    template<typename M>
//    void update_surface(typename M::surface_type& surface, M& mesh, id_type face_id_offset, id_type submesh_id);

    // Original halfedge remains associated with its original vertex
    template<typename M>
    void split_halfedge(M& mesh, id_type halfedge_id, const typename M::vertex_attributes_type& av);

    // Split at position
    // Original halfedge remains associated with its original vertex
    // Intermediate vertex attributes are interpolated from edge endpoints
    template<typename M>
    void split_halfedge(M& mesh, id_type halfedge_id, const typename M::point_type& position);

    // Original halfedges remain associated with their original vertices
    template<typename M>
    void split_edge(M& mesh, id_type halfedge_id, const typename M::vertex_attributes_type& av, const typename M::vertex_attributes_type& av_partner);

    // Split at position
    // Original halfedges remain associated with their original vertices
    // Intermediate vertex attributes are interpolated from edge endpoints
    template<typename M>
    void split_edge(M& mesh, id_type halfedge_id, const typename M::point_type& position);

    // Original halfedges remain associated with their original vertices
    // Intermediate vertex attributes are interpolated from edge endpoints
    // t ranges from 0 to 1 from vertex to end of edge
    template<typename M>
    void split_edge(M& mesh, id_type halfedge_id, const typename M::value_type& t);

    // Split into n segments
    // Original halfedges remain associated with their original vertices
    // Intermediate vertex attributes are interpolated from edge endpoints
    template<typename M>
    void split_edge(M& mesh, id_type halfedge_id, size_t n = 2);

    // id's are face halfedges whose vertices should be connected to produce two faces
    // Original halfedge and vertex id relationships are preserved
    // halfedge_id_a remains part of its original face
    // Any holes in the original face are reassigned to the new face as necessary
    // Undefined results if the split crosses a hole, but this can only happen in the general case, it will never occur with IntersectionSplitter, no need to fix right away? ...
    template<typename M>
    void split_face(M& mesh, id_type halfedge_id_a, id_type halfedge_id_b, bool coupled = true);

    // id's are face halfedges whose vertices should be connected to produce two faces with a succession of edges at the given positions
    // Original halfedge and vertex id relationships are preserved
    // halfedge_id_a remains part of its original face
    // Any holes in the original face are reassigned to the new face as necessary
    // Undefined results if the split crosses a hole, but this can only happen in the general case, it will never occur with IntersectionSplitter, no need to fix right away? ...
    template<typename M>
    void split_face(M& mesh, id_type halfedge_id_a, id_type halfedge_id_b, std::span<const typename M::point_type> positions, bool coupled = true);

    template<typename M>
    size_t hole_edge_count(const M& mesh, id_type halfedge_id);

    template<typename M>
    bool border_external(const M& mesh, id_type halfedge_id, const typename M::vector_type& normal);

    // The create_border_ functions do not calculate texcoords

    // Create a face closing off the border starting at the given halfedge; does not handle internal borders
    template<typename M>
    id_type create_border_face(M& mesh, id_type halfedge_id, const typename M::vector_type& normal, id_type surface_id = nullid);

    // Create a face closing off the border starting at the first halfedge and adding a halfedge to bridge the gap between the first and last halfedges
    template<typename M>
    id_type create_border_face(M& mesh, id_type halfedge_id_first, id_type halfedge_id_last, const typename M::vector_type& normal, id_type surface_id = nullid);

    // Create the halfedges and vertices of a face that would close off the border starting at the given border halfedge without creating a new face; handle an internal border
    template<typename M>
    id_type create_border_hole(M& mesh, id_type halfedge_id, const typename M::vector_type& normal, id_type face_id = nullid);

    // Create the halfedges and vertices of a face that would close off the border starting at the given border halfedge and with holes starting at the given halfedges
    template<typename M>
    id_type create_border_with_holes_face(M& mesh, id_type halfedge_id, const std::vector<id_type> halfedge_id_holes, const typename M::vector_type& normal, id_type surface_id = nullid);

    using BorderHalfedges = std::map<id_type, id_type>;

    // border_halfedges map each border halfedge to its submesh
    // Returns a list of the surface ids created (should not be found, assert for this?)
    template<typename M>
    std::vector<id_type> close_border(M& mesh, BorderHalfedges border_halfedges, const std::string& surface_name, const typename M::vector_type& normal);

    // Ensures that no vertices belong to more than one face (vertices are not shared between halfedges/faces)
    template<typename M>
    void prepare_face_vertices(M& mesh);

    // Call an init function at each vertex, then call an apply function for all coincident vertices
    // Captures can be used to transfer state between init and apply
    // This uses the visited flag; the caller should define the flags context and call reset_all on u at some point before calling this
    // Works with Surface, Submesh, and Mesh
    template<typename U>
    void for_each_vertex(U& u, std::function<bool(const typename U::vertex_type&)> f_init, std::function<void(typename U::vertex_type&)> f_apply);

    // Add extra template parameter or use halfedge iterators to support mixed face/hole parameters ...

    // this should set position of all vertices at each position, not just the two face vertices ...
    template<typename F>
    std::vector<std::array<id_type, 2>> unify_vertex_positions(F& face_a, F& face_b);

    // Works with pairs of faces or holes
    template<typename F>
    std::array<id_type, 2> find_partner_halfedges(const F& face_a, const F& face_b);

    // propagates position to all coincident vertices, deprecated once shared vertices are available ...
    template<typename Traits>
    void set_vertex_position(Vertex<Traits>& vertex, const typename Traits::point_type& position);

    // Returns the face hole id and the face halfedge id of the partner of the first halfedge in hole
    // {face_hole_id, face_hole_halfedge_id}
    template<typename Traits>
    id_type find_matching_hole_halfedge(const Face<Traits>& face, const typename Face<Traits>::hole_type& other_hole);

    // id's are face halfedges whose vertices should be connected with a pair of halfedges
    // No new faces are generated
    // The halfedge face id's are applied to new halfedges
    // Handles only halfedge creation and connectivity, vertex attributes at a vertex position are unchanged
    // Internal, used by split_face
    template<typename M>
    void connect(M& mesh, id_type halfedge_id_a, id_type halfedge_id_b, bool coupled = true);

    // id's are face halfedges whose vertices should be connected with pairs of halfedges
    // No new faces are generated
    // The halfedge face id's are applied to new halfedges
    // Handles only halfedge creation and connectivity, vertex attributes at a vertex position are unchanged; no, attributes other than position are undefined ...
    // Internal, used by split_face
    template<typename M>
    void connect(M& mesh, id_type halfedge_id_a, id_type halfedge_id_b, std::span<const typename M::point_type> positions, bool coupled = true);

    // Move any holes to their correct face after split_face
    // Undefined results if the split crosses a hole
    template<typename M>
    void partition_holes(M& mesh, id_type face_id_a, id_type face_id_b);

    // Move any holes in face_a to the opposite face as necessary after split_face
    // Undefined results if the split crosses a hole
    template<typename M>
    void partition_face_holes(M& mesh, typename M::face_type& face_a, typename M::face_type& face_b, std::span<id_type> hole_ids_a);

    // should be in model/mesh_util.hpp but there is none ...
    // needs to either be changed over to just use clip (preferable and correct) or manually recalculate normals, either of which will result in reference model changes ...
    // as is just replaces the current (erroneous) behavior exactly ...
    template<typename U>
    void clip_base(U& u, const typename U::traits_type::point_type& base_position, const std::string& name);

} // namespace quetzal::brep

//------------------------------------------------------------------------------
template<typename U>
void quetzal::brep::add_face_surfaces(U& u, const std::string& prefix)
{
    for (auto& face : u.faces())
    {
        std::string name = prefix + std::to_string(face.id());
        if (face.surface().name() != name)
        {
            u.add_surface_face(face.submesh_id(), name, face.id());
        }
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::remove_degenerate_face(M& mesh, id_type face_id)
{
    const auto& face = mesh.face(face_id);

    // remove_degenerate_face ...
    // order face vertices (find corners of bounding box and calculate positions of vertices along the diagonal)
    // add halfedges/vertices between each pair in sequence, connecting to successive edge and existing edges, except for those in original face

    // set degenerate face halfedge ids to nullid, then
    // mesh.delete_face(face_id);
    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::remove_degenerate_faces(M& mesh)
{
    std::vector<id_type> degenerate_ids;
    for (const auto& face : mesh.faces())
    {
        auto id = face.id();
        if (degenerate_face(mesh, id))
        {
            degenerate_ids.push_back(id);
        }
    }

    for (id_type id : degenerate_ids)
    {
        remove_degenerate_face(mesh, id);
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::brep::remove_vertex(M& mesh, id_type vertex_id)
{
    auto& vertex = mesh.vertex(vertex_id);

    id_type face_id = vertex.halfedge().face_id();

    std::vector<id_type> idsHalfedgeRemove;
    for (auto& halfedge : vertex.halfedges())
    {
        if (!halfedge.border() && halfedge.surface_id() == halfedge.partner().surface_id())
        {
            idsHalfedgeRemove.push_back(halfedge.id());
        }
    }

    for (id_type halfedge_id : idsHalfedgeRemove)
    {
        face_id = remove_edge(mesh, halfedge_id);
    }

    return face_id;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::brep::remove_edge(M& mesh, id_type halfedge_id)
{
    auto& halfedge = mesh.halfedge(halfedge_id);

    if (halfedge.border())
    {
        mesh.delete_face(halfedge.face_id());
        return nullid;
    }

    id_type face_id = halfedge.partner().face_id();

    for (auto& h : halfedge.face().halfedges())
    {
        h.set_face_id(face_id);
    }

    halfedge.next().set_prev_id(halfedge.partner().prev_id());
    halfedge.prev().set_next_id(halfedge.partner().next_id());
    halfedge.partner().next().set_prev_id(halfedge.prev_id());
    halfedge.partner().prev().set_next_id(halfedge.next_id());

    halfedge.face().set_deleted();
    halfedge.partner().set_deleted();
    halfedge.set_deleted();

    return face_id;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::update_halfedge(typename M::halfedge_type& halfedge, M& mesh, id_type halfedge_id_offset, id_type vertex_id_offset, id_type face_id_offset)
{
    halfedge.set_mesh(mesh);
    halfedge.set_id(halfedge.id() + halfedge_id_offset);
    if (halfedge.partner_id() != nullid)
    {
        halfedge.set_partner_id(halfedge.partner_id() + halfedge_id_offset);
    }
    halfedge.set_next_id(halfedge.next_id() + halfedge_id_offset);
    halfedge.set_prev_id(halfedge.prev_id() + halfedge_id_offset);
    halfedge.set_vertex_id(halfedge.vertex_id() + vertex_id_offset);
    halfedge.set_face_id(halfedge.face_id() + face_id_offset);

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::update_vertex(typename M::vertex_type& vertex, M& mesh, id_type halfedge_id_offset, id_type vertex_id_offset)
{
    vertex.set_mesh(mesh);
    vertex.set_id(vertex.id() + vertex_id_offset);
    vertex.set_halfedge_id(vertex.halfedge_id() + halfedge_id_offset);

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::update_face(typename M::face_type& face, M& mesh, id_type halfedge_id_offset, id_type face_id_offset, id_type surface_id, id_type submesh_id)
{
    face.set_mesh(mesh);
    face.set_id(face.id() + face_id_offset);
    face.set_halfedge_id(face.halfedge_id() + halfedge_id_offset);

    for (auto& hole : face.holes())
    {
        hole.set_halfedge_id(hole.halfedge_id() + halfedge_id_offset);
    }

    face.set_surface_id(surface_id);
    face.set_submesh_id(submesh_id);

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::split_halfedge(M& mesh, id_type id, const typename M::vertex_attributes_type& av)
{
    const auto& halfedge = mesh.halfedge(id);
    assert(halfedge.border());

    id_type halfedge_id0 = mesh.create_halfedge_vertex(nullid, halfedge.next_id(), id, halfedge.face_id(), av); // extension of halfedge

    // halfedge may have been invalidated
    mesh.halfedge(id).next().set_prev_id(halfedge_id0);
    mesh.halfedge(id).set_next_id(halfedge_id0);
    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::split_halfedge(M& mesh, id_type halfedge_id, const typename M::point_type& position)
{
    const auto& halfedge = mesh.halfedge(halfedge_id);
    assert(halfedge.border());

    const auto av0 = halfedge.vertex_attributes();
    const auto av1 = halfedge.next().vertex_attributes();

    auto t = to_segment(halfedge).projection_parameter(position);
    auto av = lerp(av0, av1, t);

    // Restore original position to avoid floating point precision issue or if the split point was not colinear
    assert(float_eq(av.position(), position));
    av.set_position(position);

    split_halfedge(mesh, halfedge_id, av);
    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::split_edge(M& mesh, id_type halfedge_id, const typename M::vertex_attributes_type& av, const typename M::vertex_attributes_type& av_partner)
{
    assert(av.position() == av_partner.position());

    const auto& halfedge_orig = mesh.halfedge(halfedge_id);
    id_type halfedge_id_next = halfedge_orig.next_id();
    id_type face_id_halfedge = halfedge_orig.face_id();
    id_type partner_id = halfedge_orig.partner_id();
    id_type partner_id_next = halfedge_orig.partner().next_id();
    id_type face_id_partner = halfedge_orig.partner().face_id();

    id_type halfedge_id0 = mesh.create_halfedge_vertex(partner_id, halfedge_id_next, halfedge_id, face_id_halfedge, av); // extension of halfedge
    id_type halfedge_id1 = mesh.create_halfedge_vertex(halfedge_id, partner_id_next, partner_id, face_id_partner, av_partner); // partner of halfedge

    auto& halfedge = mesh.halfedge(halfedge_id);
    halfedge.next().set_prev_id(halfedge_id0);
    halfedge.set_next_id(halfedge_id0);
    halfedge.set_partner_id(halfedge_id1);

    auto& partner = mesh.halfedge(partner_id);
    partner.next().set_prev_id(halfedge_id1);
    partner.set_next_id(halfedge_id1);
    partner.set_partner_id(halfedge_id0);
    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::split_edge(M& mesh, id_type halfedge_id, const typename M::point_type& position)
{
    const auto& halfedge = mesh.halfedge(halfedge_id);

    const auto av0 = halfedge.vertex_attributes();
    const auto av1 = halfedge.next().vertex_attributes();
    const auto avp0 = halfedge.partner().vertex_attributes();
    const auto avp1 = halfedge.partner().next().vertex_attributes();

    assert(float_eq(av0.position(), avp1.position()));
    assert(float_eq(av1.position(), avp0.position()));

    auto t = to_segment(halfedge).projection_parameter(position);
    auto av = lerp(av0, av1, t);
    auto av_partner = lerp(avp1, avp0, t);
    assert(float_eq(av.position(), av_partner.position()));

    // Restore original position and denoise texcoord to avoid floating point precision issues or if the split point was not colinear
    av.set_position(position);
    av_partner.set_position(position);
    av.texcoord().denoise01();
    av_partner.texcoord().denoise01();

    split_edge(mesh, halfedge_id, av, av_partner);
    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::split_edge(M& mesh, id_type halfedge_id, const typename M::value_type& t)
{
    const auto& halfedge = mesh.halfedge(halfedge_id);
    assert(t >= M::val(0) && t <= M::val(1));

    const auto av0 = halfedge.vertex_attributes();
    const auto av1 = halfedge.next().vertex_attributes();
    const auto avp0 = halfedge.partner().next().vertex_attributes();
    const auto avp1 = halfedge.partner().vertex_attributes();

    assert(float_eq(av0.position(), avp0.position()));
    assert(float_eq(av1.position(), avp1.position()));

    const auto av = lerp(av0, av1, t);
    const auto av_partner = lerp(avp0, avp1, t);

    // Set positions the same to avoid floating point precision issue
    av_partner.set_position(av.position());

    split_edge(mesh, halfedge_id, av, av_partner);
    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::split_edge(M& mesh, id_type halfedge_id, size_t n)
{
    const auto& halfedge = mesh.halfedge(halfedge_id);
    assert(n > 0);
    if (n == 1)
    {
        return;
    }

    const auto av0 = halfedge.vertex_attributes();
    const auto av1 = halfedge.next().vertex_attributes();
    const auto avp0 = halfedge.partner().next().vertex_attributes();
    const auto avp1 = halfedge.partner().vertex_attributes();

    assert(float_eq(av0.position(), avp0.position()));
    assert(float_eq(av1.position(), avp1.position()));

    for (size_t i = 1; i < n; ++i)
    {
        auto t = M::val(i) / M::val(n);
        auto av = lerp(av0, av1, t);
        auto av_partner = lerp(avp1, avp0, M::val(1) - t);
        assert(float_eq(av.position(), av_partner.position()));
        av_partner.set_position(av.position());
        split_edge(mesh, halfedge_id, av, av_partner);
        halfedge_id = mesh.halfedge(halfedge_id).next_id();
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::split_face(M& mesh, id_type halfedge_id_a, id_type halfedge_id_b, bool coupled)
{
    assert(halfedge_id_a != halfedge_id_b);
    assert(mesh.halfedge(halfedge_id_a).face_id() == mesh.halfedge(halfedge_id_b).face_id());
    assert(mesh.halfedge(halfedge_id_a).next_id() != halfedge_id_b);
    assert(mesh.halfedge(halfedge_id_b).next_id() != halfedge_id_a);

    id_type face_id_a = mesh.halfedge(halfedge_id_a).face_id();
    id_type surface_id = mesh.face(face_id_a).surface_id();

    mesh.face(face_id_a).set_halfedge_id(halfedge_id_a);
    id_type face_id_b = mesh.create_face(surface_id, halfedge_id_b, mesh.face(face_id_a).attributes());

    for (id_type id = halfedge_id_b; id != halfedge_id_a; id = mesh.halfedge(id).next_id())
    {
        mesh.halfedge(id).set_face_id(face_id_b);
    }

    connect(mesh, halfedge_id_a, halfedge_id_b, coupled);

    partition_holes(mesh, face_id_a, face_id_b);
    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::split_face(M& mesh, id_type halfedge_id_a, id_type halfedge_id_b, std::span<const typename M::point_type> positions, bool coupled)
{
//std::cout << std::format("split_face\t{}\t{}\t{} positions {}..{}", halfedge_id_a, halfedge_id_b, positions.size(), mesh.halfedge(halfedge_id_a).vertex_attributes().position(), mesh.halfedge(halfedge_id_b).vertex_attributes().position()) << std::endl;
//for (const auto& position : positions) std::cout << '\t' << position << std::endl;
    assert(halfedge_id_a != halfedge_id_b); // ? ...
    assert(mesh.halfedge(halfedge_id_a).face_id() == mesh.halfedge(halfedge_id_b).face_id());
    assert(positions.size() > 1);
    assert(to_segment(mesh.halfedge(halfedge_id_a)).contains(positions.front()));
    assert(to_segment(mesh.halfedge(halfedge_id_b)).contains(positions.back()));

    id_type face_id_a = mesh.halfedge(halfedge_id_a).face_id();
    id_type surface_id = mesh.face(face_id_a).surface_id();

    // split edges and reassign halfedge_id's where necessary
    if (!float_eq(positions.front(), mesh.halfedge(halfedge_id_a).vertex_attributes().position()))
    {
        split_edge(mesh, halfedge_id_a, positions.front());
        halfedge_id_a = mesh.halfedge(halfedge_id_a).next_id();
    }

    if (!float_eq(positions.back(), mesh.halfedge(halfedge_id_b).vertex_attributes().position()))
    {
        split_edge(mesh, halfedge_id_b, positions.back());
        halfedge_id_b = mesh.halfedge(halfedge_id_b).next_id();
    }

    mesh.face(face_id_a).set_halfedge_id(halfedge_id_a);
    id_type face_id_b = mesh.create_face(surface_id, halfedge_id_b, mesh.face(face_id_a).attributes());

    for (id_type id = halfedge_id_b; id != halfedge_id_a; id = mesh.halfedge(id).next_id())
    {
        mesh.halfedge(id).set_face_id(face_id_b);
    }

    assert(mesh.halfedge(halfedge_id_a).face_id() == face_id_a);
    assert(mesh.halfedge(halfedge_id_b).face_id() == face_id_b);

    connect(mesh, halfedge_id_a, halfedge_id_b, positions, coupled);

    partition_holes(mesh, face_id_a, face_id_b);
    return;
}

//------------------------------------------------------------------------------
template<typename M>
size_t quetzal::brep::hole_edge_count(const M& mesh, id_type halfedge_id)
{
    assert(mesh.valid_halfedge_id(halfedge_id));

    size_t n = 0;
    id_type id = halfedge_id;

    do
    {
        ++n;
        id = mesh.halfedge(id).next_id();
    } while (id != halfedge_id);

    return n;
}

//------------------------------------------------------------------------------
template<typename M>
bool quetzal::brep::border_external(const M& mesh, id_type halfedge_id, const typename M::vector_type& normal)
{
    assert(mesh.halfedge(halfedge_id).border());

    typename M::value_type sumAnglesExterior = M::val(0);
    id_type id = halfedge_id;

    do
    {
        const auto& halfedge = mesh.halfedge(id);

        auto positionPrev = mesh.halfedge(next_border_halfedge_id(mesh, id)).vertex_attributes().position();
        auto position = halfedge.vertex_attributes().position();
        auto positionNext = mesh.halfedge(prev_border_halfedge_id(mesh, id)).vertex_attributes().position();

        auto angleInterior = math::angle_full(positionNext - position, positionPrev - position, normal);
        sumAnglesExterior += math::Pi<typename M::value_type> - angleInterior;

        id = next_border_halfedge_id(mesh, id);
    } while (id != halfedge_id);

    return sumAnglesExterior > M::val(0);
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::brep::create_border_face(M& mesh, id_type halfedge_id, const typename M::vector_type& normal, id_type surface_id)
{
    assert(mesh.halfedge(halfedge_id).border());

    id_type nh = mesh.halfedge_store_count();
    id_type nh0 = nh;
    id_type face_id = mesh.create_face(surface_id, nh0, {normal});

    id_type id = halfedge_id;
    do
    {
        id_type partner_id = prev_border_halfedge_id(mesh, id);
        assert(partner_id != nullid);

        mesh.halfedge(partner_id).set_partner_id(nh);

        auto av = mesh.halfedge(id).vertex_attributes();
        av.set_normal(normal);

        mesh.create_halfedge_vertex(partner_id, nh + 1, nh - 1, face_id, av);

        ++nh;

        id = partner_id;
    } while (id != halfedge_id);

    // Fix first-last links
    mesh.halfedge(nh - 1).set_next_id(nh0);
    mesh.halfedge(nh0).set_prev_id(nh - 1);

    return face_id;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::brep::create_border_face(M& mesh, id_type halfedge_id_first, id_type halfedge_id_last, const typename M::vector_type& normal, id_type surface_id)
{
    assert(mesh.halfedge(halfedge_id_first).border());
    assert(mesh.halfedge(halfedge_id_last).border());
    assert(halfedge_id_first != halfedge_id_last);

    id_type nh = mesh.halfedge_store_count();
    id_type nhOrig = nh;
    id_type face_id = mesh.create_face(surface_id, nh, {normal});

    id_type id = halfedge_id_first;
    for (;;)
    {
        mesh.halfedge(id).set_partner_id(nh);
        
        auto av = mesh.halfedge(id).next().vertex_attributes();
        av.set_normal(normal);

        mesh.create_halfedge_vertex(id, nh + 1, nh - 1, face_id, av);

        ++nh;

        if (id == halfedge_id_last)
        {
            break;
        }

        id = prev_border_halfedge_id(mesh, id);
        assert(id != nullid);
    }

    // Add the connecting halfedge
    auto av = mesh.halfedge(id).vertex_attributes();
    av.set_normal(normal);

    id_type vertex_id = mesh.create_vertex(nh, av);
    mesh.create_halfedge(nullid, nhOrig, nh - 1, vertex_id, face_id);

    // Correct the end halfedge linkage
    mesh.halfedge(nh - 1).set_next_id(nh);
    mesh.halfedge(nhOrig).set_prev_id(nh);

    return face_id;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::brep::create_border_hole(M& mesh, id_type halfedge_id, const typename M::vector_type& normal, id_type face_id)
{
    assert(mesh.halfedge(halfedge_id).border());

    id_type nh = mesh.halfedge_store_count();

    id_type halfedge_id_hole = nh;
    id_type id = halfedge_id;
    do
    {
        id_type partner_id = prev_border_halfedge_id(mesh, id);

        mesh.halfedge(partner_id).set_partner_id(nh);

        auto av = mesh.halfedge(id).vertex_attributes();
        av.set_normal(normal);

        mesh.create_halfedge_vertex(partner_id, nh + 1, nh - 1, face_id, av);

        ++nh;

        id = partner_id;
    } while (id != halfedge_id);

    // Fix first-last links
    mesh.halfedge(nh - 1).set_next_id(halfedge_id_hole);
    mesh.halfedge(halfedge_id_hole).set_prev_id(nh - 1);

    mesh.face(face_id).create_hole(halfedge_id_hole);
    return halfedge_id_hole;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::id_type quetzal::brep::create_border_with_holes_face(M& mesh, id_type halfedge_id, const std::vector<id_type> halfedge_id_holes, const typename M::vector_type& normal, id_type surface_id)
{
    id_type face_id = create_border_face(mesh, halfedge_id, normal, surface_id);

    for (id_type halfedge_id_hole : halfedge_id_holes)
    {
        create_border_hole(mesh, halfedge_id_hole, normal, face_id);
    }

    return face_id;
}

//------------------------------------------------------------------------------
template<typename M>
std::vector<quetzal::id_type> quetzal::brep::close_border(M& mesh, BorderHalfedges border_halfedges, const std::string& surface_name, const typename M::vector_type& normal)
{
    assert(!border_halfedges.empty());

    // For each border polygon, store the initial halfedge in the appropriate set
    // In this function, external and internal correspond to faces and holes respectively

    using BorderHalfedge = BorderHalfedges::value_type;

    std::vector<BorderHalfedge> border_halfedges_external;
    std::vector<BorderHalfedge> border_halfedges_internal;

    while (!border_halfedges.empty())
    {
        BorderHalfedge border_halfedge = *border_halfedges.begin();
        id_type halfedge_id = border_halfedge.first;
        assert(mesh.valid_halfedge_id(halfedge_id));

        id_type id = halfedge_id;
        do
        {
            border_halfedges.erase(id);
            id = prev_border_halfedge_id(mesh, id);
        } while (id != halfedge_id);

        if (border_external(mesh, halfedge_id, normal))
        {
            border_halfedges_external.push_back(border_halfedge);
        }
        else
        {
            border_halfedges_internal.push_back(border_halfedge);
        }
    }

    assert(!border_halfedges_external.empty());

    // Associate holes with faces

    struct BorderFace
    {
        geometry::Projection<typename M::vector_traits> m_projection{};
        geometry::Polygon<typename geometry::Projection<typename M::vector_traits>::projected_traits> m_polygon{};
        typename M::value_type m_area{};
        id_type m_submesh_id = nullid;
        id_type m_halfedge_id_border_external = nullid;
        std::vector<id_type> m_halfedge_ids_border_internal{};

        bool contains(M& mesh, BorderHalfedge border_halfedge) const
        {
            id_type submesh_id = border_halfedge.second;
            assert(submesh_id != nullid);
            if (submesh_id != m_submesh_id)
            {
                return false;
            }

            // Single point check is sufficient
            auto position = m_projection(mesh.halfedge(border_halfedge.first).vertex_attributes().position());
            return m_polygon.contains(position);
        };
    };

    std::map<id_type, BorderFace> border_faces; // unordered_map ...

    if (border_halfedges_external.size() == 1)
    {
        BorderHalfedge border_halfedge_external = border_halfedges_external.front();
        id_type halfedge_id_face = border_halfedge_external.first;
        BorderFace& border_face = border_faces[halfedge_id_face];
        border_face.m_submesh_id = border_halfedge_external.second;
        border_face.m_halfedge_id_border_external = halfedge_id_face;

        std::vector<id_type> halfedge_id_border_internal;
        for (const auto& border_halfedge_internal : border_halfedges_internal)
        {
            halfedge_id_border_internal.push_back(border_halfedge_internal.first);
        }
        border_faces[halfedge_id_face].m_halfedge_ids_border_internal = halfedge_id_border_internal;
    }
    else
    {
        for (BorderHalfedge border_halfedge : border_halfedges_external)
        {
            id_type halfedge_id_face = border_halfedge.first;
            BorderFace& border_face = border_faces[halfedge_id_face];
            border_face.m_submesh_id = border_halfedge.second;
            border_face.m_halfedge_id_border_external = halfedge_id_face;
            border_face.m_projection.init(normal);
            border_face.m_polygon = to_border_polygon(mesh, halfedge_id_face, border_face.m_projection);
            assert(border_face.m_polygon.vertex_count() > 0);
            border_face.m_area = border_face.m_polygon.area();
            assert(border_face.m_area > M::val(0));
        }

        // Associate each hole with its enclosing face
        for (BorderHalfedge border_halfedge_internal : border_halfedges_internal)
        {
            id_type halfedge_id_hole  = border_halfedge_internal.first;
            id_type halfedge_id_face_min = nullid;
            typename M::value_type areaMin = M::val(0);

            for (BorderHalfedge border_halfedge_external : border_halfedges_external)
            {
                id_type halfedge_id_face  = border_halfedge_external.first;
                const auto& border_face = border_faces[halfedge_id_face];
                if (border_face.contains(mesh, border_halfedge_internal))
                {
                    auto area = border_face.m_area;
                    if (halfedge_id_face_min == nullid || area < areaMin)
                    {
                        halfedge_id_face_min = halfedge_id_face;
                        areaMin = area;
                    }
                }
            }

            assert(halfedge_id_face_min != nullid);
            border_faces[halfedge_id_face_min].m_halfedge_ids_border_internal.push_back(halfedge_id_hole);
        }
    }

    assert(!border_faces.empty());

    // Close off the border with a new face and holes

    std::vector<id_type> idsSurface;

    for (const auto& value : border_faces)
    {
        const BorderFace& border_face = value.second;
        id_type submesh_id = border_face.m_submesh_id;
        id_type surface_id = mesh.get_surface_id(submesh_id, surface_name, {normal});
        idsSurface.push_back(surface_id);
        id_type face_id = create_border_with_holes_face(mesh, border_face.m_halfedge_id_border_external, border_face.m_halfedge_ids_border_internal, normal, surface_id);

        if constexpr (M::vertex_attributes_type::contains(geometry::AttributesFlags::Texcoord0))
        {
            calculate_face_texcoords(mesh, face_id);
        }
    }

    return idsSurface;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::prepare_face_vertices(M& mesh)
{
    typename M::vertices_type vertices_orig = move(mesh.vertex_store());
    mesh.vertex_store().clear();

    for (auto& face : mesh.faces())
    {
        for (auto& halfedge : face.halfedges())
        {
            halfedge.set_vertex_id(mesh.create_vertex(halfedge.id(), vertices_orig[halfedge.vertex_id()].vertex_attributes()));
        }
    }

    return;
}

//------------------------------------------------------------------------------
template<typename U>
void quetzal::brep::for_each_vertex(U& u, std::function<bool(const typename U::vertex_type&)> f_init, std::function<void(typename U::vertex_type&)> f_apply)
{
    u.reset_all();

    for (auto& face : u.faces())
    {
        for (auto& halfedge : face.halfedges())
        {
            auto& vertex = halfedge.vertex();
            if (vertex.visited())
            {
                continue;
            }

            if (!f_init(vertex))
            {
                continue;
            }

            for (auto& h : vertex.halfedges())
            {
                if (h.surface_id() != halfedge.surface_id()) // Respect surface boundaries
                {
                    continue;
                }

                f_apply(h.vertex());
                h.vertex().set_visited();
            }
        }
    }

    return;
}

//------------------------------------------------------------------------------
template<typename F>
std::vector<std::array<quetzal::id_type, 2>> quetzal::brep::unify_vertex_positions(F& face_a, F& face_b)
{
    assert(face_a.halfedge_count() == face_b.halfedge_count());

    auto [halfedge_id_a, halfedge_id_b] = find_partner_halfedges(face_a, face_b);

    auto& mesh = face_a.mesh();
    size_t n = face_a.halfedge_count();
    std::vector<std::array<id_type, 2>> ids;

    for (size_t i = 0; i < n; ++i)
    {
        ids.push_back({halfedge_id_a, halfedge_id_b});

        auto& halfedge_a = mesh.halfedge(halfedge_id_a);
        auto& halfedge_b = mesh.halfedge(halfedge_id_b);

        auto position = halfedge_a.vertex_attributes().position();

        set_vertex_position(halfedge_a.vertex(), position);
        set_vertex_position(halfedge_b.next().vertex(), position);

        halfedge_id_a = halfedge_a.next_id();
        halfedge_id_b = halfedge_b.prev_id();
    }

    return ids;
}

//------------------------------------------------------------------------------
template<typename F>
std::array<quetzal::id_type, 2> quetzal::brep::find_partner_halfedges(const F& face_a, const F& face_b)
{
    const auto& halfedge_a = face_a.halfedge();
    for (const auto& halfedge_b : face_b.halfedges())
    {
        if (coincident(halfedge_a.vertex_attributes(), halfedge_b.next().vertex_attributes()) && coincident(halfedge_a.next().vertex_attributes(), halfedge_b.vertex_attributes()))
        {
            return {halfedge_a.id(), halfedge_b.id()};
        }
    }

    return {nullid, nullid};
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::brep::set_vertex_position(Vertex<Traits>& vertex, const typename Traits::point_type& position)
{
    for (auto& halfedge : vertex.halfedges())
    {
        assert(float_eq(halfedge.vertex_attributes().position(), position));
        halfedge.vertex_attributes().set_position(position);
    }

    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::id_type quetzal::brep::find_matching_hole_halfedge(const Face<Traits>& face, const typename Face<Traits>::hole_type& other_hole)
{
    const auto& other_halfedge = other_hole.halfedges().front();

    for (const auto& hole : face.holes())
    {
        for (const auto& halfedge : hole.halfedges())
        {
            if (coincident(halfedge.vertex_attributes(), other_halfedge.next().vertex_attributes()) && coincident(halfedge.next().vertex_attributes(), other_halfedge.vertex_attributes()))
            {
                return halfedge.id();
            }
        }
    }

    return nullid;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::connect(M& mesh, id_type halfedge_id_a, id_type halfedge_id_b, bool coupled)
{
    assert(halfedge_id_a != halfedge_id_b);

    id_type nh = mesh.halfedge_store_count();

    auto& halfedge_a = mesh.halfedge(halfedge_id_a);
    id_type halfedge_id_a_prev = halfedge_a.prev_id();
    halfedge_a.prev().set_next_id(nh + 1);
    halfedge_a.set_prev_id(nh);
    auto av_b = halfedge_a.vertex_attributes();

    auto& halfedge_b = mesh.halfedge(halfedge_id_b);
    id_type halfedge_id_b_prev = halfedge_b.prev_id();
    halfedge_b.prev().set_next_id(nh);
    halfedge_b.set_prev_id(nh + 1);
    auto av_a = halfedge_b.vertex_attributes();

    id_type partner_id_a = coupled ? nh + 1 : nullid;
    id_type partner_id_b = coupled ? nh : nullid;

    id_type face_id_a = mesh.halfedge(halfedge_id_a).face_id();
    id_type face_id_b = mesh.halfedge(halfedge_id_b).face_id();

    mesh.create_halfedge_vertex(partner_id_a, halfedge_id_a, halfedge_id_b_prev, face_id_a, av_a);
    mesh.create_halfedge_vertex(partner_id_b, halfedge_id_b, halfedge_id_a_prev, face_id_b, av_b);
    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::connect(M& mesh, id_type halfedge_id_a, id_type halfedge_id_b, std::span<const typename M::point_type> positions, bool coupled)
{
assert(false); // unused ...

    assert(halfedge_id_a != halfedge_id_b);

    auto& halfedge_a = mesh.halfedge(halfedge_id_a);
    assert(float_eq(halfedge_a.vertex_attributes().position(), positions.front()));

    auto& halfedge_b = mesh.halfedge(halfedge_id_b);
    assert(float_eq(halfedge_b.vertex_attributes().position(), positions.back()));

    id_type face_id_a = halfedge_a.face_id();
    const auto& face_a = mesh.face(face_id_a);

    id_type face_id_b = halfedge_b.face_id();
    const auto& face_b = mesh.face(face_id_b);

    typename M::vertex_attributes_type avA{};
    typename M::vertex_attributes_type avB{};

    avA.set_normal(face_a.attributes().normal());
    avB.set_normal(face_b.attributes().normal());

    id_type nh = mesh.halfedge_store_count();
    id_type nhOrig = nh;

    id_type halfedge_id_a_next = halfedge_id_a;
    id_type halfedge_id_b_prev = halfedge_a.prev_id();

    for (auto i = std::next(positions.begin()); i != positions.end(); ++i)
    {
        auto position = *i;
        avA.set_position(position);
        avB.set_position(mesh.halfedge(halfedge_id_a_next).vertex_attributes().position());

        id_type partner_id_a = coupled ? nh + 1 : nullid;
        id_type partner_id_b = coupled ? nh : nullid;

        mesh.create_halfedge_vertex(partner_id_a, halfedge_id_a_next, nh + 2, face_id_a, avA);
        mesh.create_halfedge_vertex(partner_id_b, nh + 3, halfedge_id_b_prev, face_id_b, avB);

        halfedge_id_a_next = nh;
        halfedge_id_b_prev = nh + 1;
        nh += 2;
    }

    mesh.halfedge(halfedge_id_a_next).set_prev_id(mesh.halfedge(halfedge_id_b).prev_id());
    mesh.halfedge(halfedge_id_b_prev).set_next_id(halfedge_id_b);

    mesh.halfedge(halfedge_id_a).prev().set_next_id(nhOrig + 1);
    mesh.halfedge(halfedge_id_a).set_prev_id(nhOrig);

    mesh.halfedge(halfedge_id_b).prev().set_next_id(halfedge_id_a_next);
    mesh.halfedge(halfedge_id_b).set_prev_id(halfedge_id_b_prev);

    if constexpr (M::vertex_attributes_type::contains(geometry::AttributesFlags::Texcoord0))
    {
if constexpr (false) // ...
{
        geometry::Projection<typename M::vector_traits> projection(face_a.attributes().normal());
        auto mapping = position_texcoord_mapping1(face_a, projection);

        for (id_type id_a = mesh.halfedge(halfedge_id_a).prev_id(); mesh.halfedge(id_a).partner_id() != mesh.halfedge(halfedge_id_b).prev_id(); id_a = mesh.halfedge(id_a).prev_id())
        {
            auto& halfedge = mesh.halfedge(id_a);
            auto texcoord = mapping(projection(halfedge.vertex_attributes().position())); // same for both since originally a single face
            halfedge.vertex_attributes().set_texcoord(texcoord);

            if (!halfedge.border())
            {
                halfedge.partner().next().vertex_attributes().set_texcoord(texcoord);
            }
        }
}
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::partition_holes(M& mesh, id_type face_id_a, id_type face_id_b)
{
    auto& face_a = mesh.face(face_id_a);
    auto& face_b = mesh.face(face_id_b);

    std::vector<id_type> hole_halfedge_ids_a = face_a.holes()
        | std::views::transform([](const Hole<typename M::traits_type>& hole) { return hole.halfedge_id(); })
        | std::ranges::to<std::vector>();
    std::vector<id_type> hole_halfedge_ids_b = face_b.holes()
        | std::views::transform([](const Hole<typename M::traits_type>& hole) { return hole.halfedge_id(); })
        | std::ranges::to<std::vector>();

    partition_face_holes(mesh, face_a, face_b, hole_halfedge_ids_a);
    partition_face_holes(mesh, face_b, face_a, hole_halfedge_ids_b);
    return;
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::brep::partition_face_holes(M& mesh, typename M::face_type& face_a, typename M::face_type& face_b, std::span<id_type> hole_halfedge_ids_a)
{
    for (id_type hole_halfedge_id_a : hole_halfedge_ids_a)
    {
        if (face_contains(face_a, vertex_position(mesh.halfedge(hole_halfedge_id_a))))
        {
            continue;
        }

        // Hole is no longer in face_a, so move it to face_b
        auto hole = face_a.halfedge_hole(hole_halfedge_id_a);
        face_a.remove_hole(hole_halfedge_id_a);
        hole.set_face_id(face_b.id());
        face_b.insert_hole(hole);
    }

    return;
}

//------------------------------------------------------------------------------
template<typename U>
void quetzal::brep::clip_base(U& u, const typename U::traits_type::point_type& base_position, const std::string& name)
{
    u.reset_all();

    std::set<id_type> face_ids;
    for_each_vertex(u,
        [base_position](const auto& vertex) -> bool
        {
            return math::float_le(vertex.attributes().position().z(), base_position.z());
        },
        [base_position, &face_ids](auto& vertex)
        {
            vertex.attributes().position().set_z(base_position.z());
            face_ids.insert(vertex.halfedge().face_id());
        }
    );
/*
    u.reset_all();

    for each face in surface name
        auto normal = geometry::normal(face);
        face.set_normal(normal);
    for_each_vertex(u.surface(name)
        init calculate vertex normal
        apply vertex_normal
*/
// the better way ...
//    geometry::HalfSpace<vector_traits> hs({pase_position, -vector_type::z_direction()});
//    clip(mesh.submesh(submesh_id), hs, "base");
name;
    return;
}

#endif // QUETZAL_BREP_MESH_UTIL_HPP
