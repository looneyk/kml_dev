#if !defined(QUETZAL_BREP_HALFEDGE_HPP)
#define QUETZAL_BREP_HALFEDGE_HPP
//------------------------------------------------------------------------------
// brep
// Halfedge.hpp
//------------------------------------------------------------------------------

#include "id.hpp"
#include "quetzal/common/Flags.hpp"
#include "quetzal/common/Properties.hpp"
#include "quetzal/geometry/AxisAlignedBoundingBox.hpp"
#include <iostream>
#include <cassert>

namespace quetzal::brep
{

    template<typename Traits>
    class Mesh;

    //--------------------------------------------------------------------------
    template<typename Traits, typename M = Mesh<Traits>>
    class Halfedge : public Flags
    {
    public:

        using traits_type = Traits;
        using mesh_type = M;
        using vertex_type = mesh_type::vertex_type;
        using face_type = mesh_type::face_type;
        using vertex_attributes_type = mesh_type::vertex_attributes_type;
        using face_attributes_type = mesh_type::face_attributes_type;

        Halfedge();
        Halfedge(mesh_type& mesh, id_type id, id_type idPartner = nullid, id_type idNext = nullid, id_type idPrev = nullid, id_type idVertex = nullid, id_type idFace = nullid, const Properties& properties = {});
        Halfedge(const Halfedge&) = default;
        Halfedge(Halfedge&&) noexcept = default;
        ~Halfedge() = default;

        Halfedge& operator=(const Halfedge&) = default;
        Halfedge& operator=(Halfedge&&) = default;

        const mesh_type& mesh() const;
        mesh_type& mesh();

        id_type id() const;

        id_type partner_id() const;
        void set_partner_id(id_type idHalfedge);

        const Halfedge& partner() const;
        Halfedge& partner();

        id_type next_id() const;
        void set_next_id(id_type idHalfedge);

        const Halfedge& next() const;
        Halfedge& next();

        id_type prev_id() const;
        void set_prev_id(id_type idHalfedge);

        const Halfedge& prev() const;
        Halfedge& prev();

        id_type vertex_id() const;
        void set_vertex_id(id_type idVertex);

        const vertex_type& vertex() const;
        vertex_type& vertex();

        const vertex_attributes_type& vertex_attributes() const;
        vertex_attributes_type& vertex_attributes();
        void set_vertex_attributes(const vertex_attributes_type& attributes);

        id_type face_id() const;
        void set_face_id(id_type idFace);

        const face_type& face() const;
        face_type& face();

        id_type partner_face_id() const;

        const face_attributes_type& face_attributes() const;
        face_attributes_type& face_attributes();
        void set_face_attributes(const face_attributes_type& attributes);

        // When the halfedge is part of a hole in its face
        id_type hole_id() const;
        void set_hole_id(id_type idHole);

        bool border() const;
        void set_border();
        void uncouple(); // severs adjacency, cleave partner relationships, this and any partner are now border halfedges

        bool surface_border() const;
        id_type surface_id() const;
        id_type partner_surface_id() const;

        const Properties& properties() const;
        Properties& properties();

        const geometry::AxisAlignedBoundingBox<typename M::vector_traits> bounds() const;

        size_t error_count() const;
        bool check() const;

        // Internal use
        void set_mesh(mesh_type& mesh);
        void set_id(id_type id);
        void check_mesh(const mesh_type* const pmesh) const;

    private:

        mesh_type* m_pmesh;
        id_type m_id;
        id_type m_idPartner;
        id_type m_idNext;
        id_type m_idPrev;
        id_type m_idVertex;
        id_type m_idFace;
        id_type m_idHole;
        Properties m_properties;
    };

    template<typename Traits, typename M>
    std::ostream& operator<<(std::ostream& os, const Halfedge<Traits, M>& halfedge);

} // namespace quetzal::brep

//------------------------------------------------------------------------------
template<typename Traits, typename M>
quetzal::brep::Halfedge<Traits, M>::Halfedge() :
    Flags(),
    m_pmesh(nullptr),
    m_id(nullid),
    m_idPartner(nullid),
    m_idNext(nullid),
    m_idPrev(nullid),
    m_idVertex(nullid),
    m_idFace(nullid),
    m_idHole(nullid),
    m_properties()
{
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
quetzal::brep::Halfedge<Traits, M>::Halfedge(mesh_type& mesh, id_type id, id_type idPartner, id_type idNext, id_type idPrev, id_type idVertex, id_type idFace, const Properties& properties) :
    Flags(),
    m_pmesh(&mesh),
    m_id(id),
    m_idPartner(idPartner),
    m_idNext(idNext),
    m_idPrev(idPrev),
    m_idVertex(idVertex),
    m_idFace(idFace),
    m_idHole(nullid),
    m_properties(properties)
{
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Halfedge<Traits, M>::mesh_type& quetzal::brep::Halfedge<Traits, M>::mesh()
{
    assert(m_pmesh != nullptr);
    return *m_pmesh;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
const typename quetzal::brep::Halfedge<Traits, M>::mesh_type& quetzal::brep::Halfedge<Traits, M>::mesh() const
{
    assert(m_pmesh != nullptr);
    return *m_pmesh;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
quetzal::id_type quetzal::brep::Halfedge<Traits, M>::id() const
{
    return m_id;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
quetzal::id_type quetzal::brep::Halfedge<Traits, M>::partner_id() const
{
    return m_idPartner;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Halfedge<Traits, M>::set_partner_id(id_type idHalfedge)
{
    m_idPartner = idHalfedge;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
const quetzal::brep::Halfedge<Traits, M>& quetzal::brep::Halfedge<Traits, M>::partner() const
{
    assert(m_pmesh != nullptr);
    return m_pmesh->halfedge(m_idPartner);
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Halfedge<Traits, M>& quetzal::brep::Halfedge<Traits, M>::partner()
{
    assert(m_pmesh != nullptr);
    return m_pmesh->halfedge(m_idPartner);
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
quetzal::id_type quetzal::brep::Halfedge<Traits, M>::next_id() const
{
    return m_idNext;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Halfedge<Traits, M>::set_next_id(id_type idHalfedge)
{
    m_idNext = idHalfedge;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
const quetzal::brep::Halfedge<Traits, M>& quetzal::brep::Halfedge<Traits, M>::next() const
{
    assert(m_pmesh != nullptr);
    return m_pmesh->halfedge(m_idNext);
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Halfedge<Traits, M>& quetzal::brep::Halfedge<Traits, M>::next()
{
    assert(m_pmesh != nullptr);
    return m_pmesh->halfedge(m_idNext);
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
quetzal::id_type quetzal::brep::Halfedge<Traits, M>::prev_id() const
{
    return m_idPrev;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Halfedge<Traits, M>::set_prev_id(id_type idHalfedge)
{
    m_idPrev = idHalfedge;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
const quetzal::brep::Halfedge<Traits, M>& quetzal::brep::Halfedge<Traits, M>::prev() const
{
    assert(m_pmesh != nullptr);
    return m_pmesh->halfedge(m_idPrev);
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Halfedge<Traits, M>& quetzal::brep::Halfedge<Traits, M>::prev()
{
    assert(m_pmesh != nullptr);
    return m_pmesh->halfedge(m_idPrev);
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
quetzal::id_type quetzal::brep::Halfedge<Traits, M>::vertex_id() const
{
    return m_idVertex;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Halfedge<Traits, M>::set_vertex_id(id_type idVertex)
{
    m_idVertex = idVertex;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
const typename quetzal::brep::Halfedge<Traits, M>::vertex_type& quetzal::brep::Halfedge<Traits, M>::vertex() const
{
    assert(m_pmesh != nullptr);
    return m_pmesh->vertex_store()[m_idVertex]; // bypass deleted check, maybe we don't want to do that in .vertex(i)? can of worms ...
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Halfedge<Traits, M>::vertex_type& quetzal::brep::Halfedge<Traits, M>::vertex()
{
    assert(m_pmesh != nullptr);
    return m_pmesh->vertex_store()[m_idVertex];
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
const typename quetzal::brep::Halfedge<Traits, M>::vertex_attributes_type& quetzal::brep::Halfedge<Traits, M>::vertex_attributes() const
{
    assert(m_pmesh != nullptr);
    return m_pmesh->vertex(m_idVertex).attributes();
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Halfedge<Traits, M>::vertex_attributes_type& quetzal::brep::Halfedge<Traits, M>::vertex_attributes()
{
    assert(m_pmesh != nullptr);
    return m_pmesh->vertex(m_idVertex).attributes();
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Halfedge<Traits, M>::set_vertex_attributes(const vertex_attributes_type& attributes)
{
    assert(m_pmesh != nullptr);
    m_pmesh->vertex(m_idVertex).set_attributes(attributes);
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
quetzal::id_type quetzal::brep::Halfedge<Traits, M>::face_id() const
{
    return m_idFace;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Halfedge<Traits, M>::set_face_id(id_type idFace)
{
    m_idFace = idFace;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
const typename quetzal::brep::Halfedge<Traits, M>::face_type& quetzal::brep::Halfedge<Traits, M>::face() const
{
    assert(m_pmesh != nullptr);
    return m_pmesh->face(m_idFace);
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Halfedge<Traits, M>::face_type& quetzal::brep::Halfedge<Traits, M>::face()
{
    assert(m_pmesh != nullptr);
    return m_pmesh->face(m_idFace);
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
quetzal::id_type quetzal::brep::Halfedge<Traits, M>::partner_face_id() const
{
    if (border())
    {
        return nullid;
    }

    return partner().face_id();
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
const typename quetzal::brep::Halfedge<Traits, M>::face_attributes_type& quetzal::brep::Halfedge<Traits, M>::face_attributes() const
{
    assert(m_pmesh != nullptr);
    return m_pmesh->face(m_idFace).attributes();
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Halfedge<Traits, M>::face_attributes_type& quetzal::brep::Halfedge<Traits, M>::face_attributes()
{
    assert(m_pmesh != nullptr);
    return m_pmesh->face(m_idFace).attributes();
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Halfedge<Traits, M>::set_face_attributes(const face_attributes_type& attributes)
{
    assert(m_pmesh != nullptr);
    m_pmesh->face(m_idFace).set_attributes(attributes);
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
quetzal::id_type quetzal::brep::Halfedge<Traits, M>::hole_id() const
{
    return m_idHole;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Halfedge<Traits, M>::set_hole_id(id_type idHole)
{
    m_idHole = idHole;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
bool quetzal::brep::Halfedge<Traits, M>::border() const
{
    return m_idPartner == nullid;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Halfedge<Traits, M>::set_border()
{
    m_idPartner = nullid;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Halfedge<Traits, M>::uncouple()
{
    if (!border())
    {
        partner().set_border();
        set_border();
    }
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
bool quetzal::brep::Halfedge<Traits, M>::surface_border() const
{
    return surface_id() != partner_surface_id();
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
quetzal::id_type quetzal::brep::Halfedge<Traits, M>::surface_id() const
{
    return face().surface_id();
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
quetzal::id_type quetzal::brep::Halfedge<Traits, M>::partner_surface_id() const
{
    if (border())
    {
        return nullid;
    }

    return partner().face().surface_id();
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename const quetzal::Properties& quetzal::brep::Halfedge<Traits, M>::properties() const
{
    return m_properties;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::Properties& quetzal::brep::Halfedge<Traits, M>::properties()
{
    return m_properties;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
const quetzal::geometry::AxisAlignedBoundingBox<typename M::vector_traits> quetzal::brep::Halfedge<Traits, M>::bounds() const
{
    // This is so trivial that it is not worth tracking dirty state and caching the result
    geometry::AxisAlignedBoundingBox<typename M::vector_traits> bounds;
    bounds.insert(vertex_attributes().position());
    bounds.insert(next().vertex_attributes().position());
    return bounds;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
size_t quetzal::brep::Halfedge<Traits, M>::error_count() const
{
    assert(m_pmesh != nullptr);
    size_t n = 0;

    if (!deleted())
    {
        n += m_id < m_pmesh->halfedge_store_count() ? 0 : 1;
        n += m_pmesh->valid_halfedge_id(m_idPartner) ? 0 : 1;
        n += m_pmesh->valid_halfedge_id(m_idNext) ? 0 : 1;
        n += m_pmesh->valid_halfedge_id(m_idPrev) ? 0 : 1;
        n += m_pmesh->valid_vertex_id(m_idVertex) ? 0 : 1;
        n += m_pmesh->valid_face_id(m_idFace) ? 0 : 1;
    }

    return n;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
bool quetzal::brep::Halfedge<Traits, M>::check() const
{
    return error_count() == 0;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Halfedge<Traits, M>::set_mesh(mesh_type& mesh)
{
    m_pmesh = &mesh;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Halfedge<Traits, M>::set_id(id_type id)
{
    m_id = id;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Halfedge<Traits, M>::check_mesh(const mesh_type* const pmesh) const
{
    assert(m_pmesh == pmesh);
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
std::ostream& quetzal::brep::operator<<(std::ostream& os, const Halfedge<Traits, M>& halfedge)
{
    os << static_cast<Flags>(halfedge);
    os << '\t' << halfedge.id();

    if (!halfedge.deleted())
    {
        os << '\t';
        if (halfedge.partner_id() != nullid)
        {
            os << halfedge.partner_id();
        }
        else
        {
            os << "border";
        }

        os << '\t' << halfedge.next_id();
        os << '\t' << halfedge.prev_id();
        os << '\t' << halfedge.vertex_id();
        os << '\t' << halfedge.face_id();
        if (halfedge.vertex_id() != nullid)
        {
            os << '\t' << halfedge.vertex_attributes();
        }
        else
        {
            os << "\tvertex null";
        }

        os << '\t' << halfedge.properties();
    }

    os << '\n';
    return os;
}

#endif // QUETZAL_BREP_HALFEDGE_HPP
