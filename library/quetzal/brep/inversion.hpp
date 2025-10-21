#if !defined(QUETZAL_BREP_INVERSION_HPP)
#define QUETZAL_BREP_INVERSION_HPP
//------------------------------------------------------------------------------
// brep
// inversion.hpp
//------------------------------------------------------------------------------

#include "Mesh.hpp"
#include "id.hpp"
#include "quetzal/geometry/Attributes.hpp"

namespace quetzal::brep
{

    template<typename Traits>
    using transform_texcoord_type = geometry::Attributes<typename Traits::vector_traits>::transform_texcoord_type;

    // Reverses orientation of each face using invert function of attributes type
    // Works with Mesh and Submesh
    template<typename Traits>
    void invert(Mesh<Traits>& mesh, transform_texcoord_type<Traits> transform_texcoord = geometry::Attributes<typename Traits::vector_traits>::transform_texcoord_reflect_u);

    template<typename Traits>
    void invert(Submesh<Traits>& submesh, transform_texcoord_type<Traits> transform_texcoord = geometry::Attributes<typename Traits::vector_traits>::transform_texcoord_reflect_u);

    template<typename Traits>
    void invert(Surface<Traits>& surface, transform_texcoord_type<Traits> transform_texcoord = geometry::Attributes<typename Traits::vector_traits>::transform_texcoord_reflect_u);

    template<typename Traits>
    void invert(Face<Traits>& face, transform_texcoord_type<Traits> transform_texcoord = geometry::Attributes<typename Traits::vector_traits>::transform_texcoord_reflect_u);

    // Reverses orientation of each face using invert function of attributes type
    // Reassigns vertices leaving partners unchanged
    // (this could be grouped with other border functions in border.hpp, but having it here localizes transform_texcoord ...)
    template<typename Traits>
    void invert_border_fragment(Mesh<Traits>& mesh, id_type halfedge_id, transform_texcoord_type<Traits> transform_texcoord = geometry::Attributes<typename Traits::vector_traits>::transform_texcoord_reflect_u);

    template<typename Traits>
    void invert_border_fragment_internal(Mesh<Traits>& mesh, id_type halfedge_id, transform_texcoord_type<Traits> transform_texcoord = geometry::Attributes<typename Traits::vector_traits>::transform_texcoord_reflect_u);

    template<typename Traits>
    void reverse_winding_order(Face<Traits>& face);

} // namespace quetzal::brep

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::brep::invert(Mesh<Traits>& mesh, transform_texcoord_type<Traits> transform_texcoord)
{
    for (auto& surface : mesh.surfaces())
    {
        invert(surface, transform_texcoord);
    }

    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::brep::invert(Submesh<Traits>& submesh, transform_texcoord_type<Traits> transform_texcoord)
{
    for (auto& surface : submesh.surfaces())
    {
        invert(surface, transform_texcoord);
    }

    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::brep::invert(Surface<Traits>& surface, transform_texcoord_type<Traits> transform_texcoord)
{
    for (auto& face : surface.faces())
    {
        invert(face, transform_texcoord);
    }

    surface.attributes().invert();
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::brep::invert(Face<Traits>& face, transform_texcoord_type<Traits> transform_texcoord)
{
    assert(!face.deleted());

    for (auto& halfedge : face.halfedges())
    {
        halfedge.vertex_attributes().invert(transform_texcoord);
    }

    face.attributes().invert();
    reverse_winding_order(face);
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::brep::invert_border_fragment(Mesh<Traits>& mesh, id_type halfedge_id, transform_texcoord_type<Traits> transform_texcoord)
{
    mesh.reset_all();
    invert_border_fragment_internal(mesh, halfedge_id, transform_texcoord);
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::brep::invert_border_fragment_internal(Mesh<Traits>& mesh, id_type halfedge_id, transform_texcoord_type<Traits> transform_texcoord)
{
    id_type face_id = mesh.halfedge(halfedge_id).face_id();
    invert(mesh.face(face_id), transform_texcoord);
    mesh.face(face_id).set_marked();

    for (const auto& halfedge : mesh.face(face_id).halfedges())
    {
        if (!halfedge.border())
        {
            if (!halfedge.partner().face().marked())
            {
                invert_border_fragment_internal(mesh, halfedge.partner_id(), transform_texcoord);
            }
        }
    }

    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::brep::reverse_winding_order(Face<Traits>& face)
{
    assert(!face.deleted());
    assert(face.halfedge_count() >= 3);

    auto& mesh = face.mesh();

    const id_type halfedge_id0 = face.halfedge_id();
    const id_type vertex_id0 = face.halfedge().vertex_id();

    id_type halfedge_id_next = halfedge_id0;
    do
    {
        auto& halfedge = mesh.halfedge(halfedge_id_next);
        halfedge_id_next = halfedge.next_id();

        id_type vertex_id = halfedge_id_next == halfedge_id0 ? vertex_id0 : halfedge.next().vertex_id();
        mesh.vertex(vertex_id).set_halfedge_id(halfedge.id());

        halfedge.set_vertex_id(vertex_id);
        halfedge.set_next_id(halfedge.prev_id());
        halfedge.set_prev_id(halfedge_id_next);
    }
    while (halfedge_id_next != halfedge_id0);

    return;
}

#endif // QUETZAL_BREP_INVERSION_HPP
