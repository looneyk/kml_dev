#if !defined(QUETZAL_BREP_VERTEX_HPP)
#define QUETZAL_BREP_VERTEX_HPP
//------------------------------------------------------------------------------
// brep
// Vertex.hpp
//------------------------------------------------------------------------------

#include "id.hpp"
#include "quetzal/common/Elements.hpp"
#include "quetzal/common/Flags.hpp"
#include "quetzal/common/Properties.hpp"
#include <iostream>
#include <set>
#include <cassert>

namespace quetzal::brep
{

    template<typename Traits>
    class Mesh;

    //--------------------------------------------------------------------------
    template<typename Traits, typename M = Mesh<Traits>>
    class Vertex : public Flags
    {
    public:

        using traits_type = Traits;
        using mesh_type = M;
        using halfedge_type = mesh_type::halfedge_type;
        using halfedge_ids_type = std::set<id_type>;
        using halfedges_type = Elements<mesh_type, halfedge_type, id_type>;
        using attributes_type = mesh_type::vertex_attributes_type;
        using size_type = mesh_type::size_type;

        Vertex();
        Vertex(mesh_type& mesh, id_type id, id_type idHalfedge, const attributes_type& attributes, const Properties& properties = {});
        Vertex(const Vertex&) = default;
        Vertex(Vertex&&) noexcept = default;
        ~Vertex() = default;

        Vertex& operator=(const Vertex&) = default;
        Vertex& operator=(Vertex&&) = default;

        const mesh_type& mesh() const;
        mesh_type& mesh();

        id_type id() const;

        // deprecated in favor of iterators below, no longer necessarily 1-1 ...
        id_type halfedge_id() const;
        void set_halfedge_id(id_type id);

        // deprecated in favor of iterators below, no longer necessarily 1-1 ...
        const halfedge_type& halfedge() const;
        halfedge_type& halfedge();

        // Virtual container interface, iterators for halfedges based at this vertex position
        // This is not currently dependable if there are borders in the mesh, but should at least account for all in this vertex's surface ...
        // This should return halfedges in CCW order (as is now, but also in future) ...

        size_type halfedge_count() const;

        const halfedges_type& halfedges() const;
        halfedges_type& halfedges();

        const halfedge_type& halfedge(id_type id) const;
        halfedge_type& halfedge(id_type id);
/*
        size_type halfedge_count(id_type surface_id) const;

        const halfedges_type& halfedges(id_type surface_id) const;
        halfedges_type& halfedges(id_type surface_id);
*/
        // the following functions are here for future use, and may change before being implemented ...
        // eventually there should be a 1-1 mapping between halfedges() and halfedge_ids(), and they should encompass all coincident halfedge vertices irrespective of surface or border presence ...

        const halfedge_ids_type& halfedge_ids() const;
        halfedge_ids_type& halfedge_ids();

        const halfedge_ids_type& halfedge_ids(id_type surface_id) const;
        halfedge_ids_type& halfedge_ids(id_type surface_id);

        bool contains_halfedge(id_type id) const;
        void insert_halfedge(id_type id); // add halfedge to list
        void link_halfedge(id_type id); // add halfedge to list and set halfedge vertex id
        void unlink_halfedge(id_type id); // remove halfedge from list and clear halfedge vertex id

        const attributes_type& attributes() const;
        attributes_type& attributes();
        void set_attributes(const attributes_type& attributes);

        const Properties& properties() const;
        Properties& properties();

        bool check() const;

        // Internal use
        void set_mesh(mesh_type& mesh);
        void set_id(id_type id);
        void check_mesh(const mesh_type* const pmesh) const;

    private:

        mesh_type* m_pmesh;
        id_type m_id;
        id_type m_idHalfedge; // no longer meaningful once/if halfedge_ids implemented ...
        halfedge_ids_type m_halfedge_ids;
        attributes_type m_attributes;
        Properties m_properties;

        halfedges_type m_halfedges;

        static halfedges_type::size_function_type m_halfedges_size;
        static halfedges_type::terminal_function_type m_halfedges_first;
        static halfedges_type::terminal_function_type m_halfedges_last;
        static halfedges_type::terminal_function_type m_halfedges_end;
        static halfedges_type::iterate_function_type m_halfedges_forward;
        static halfedges_type::iterate_function_type m_halfedges_reverse;
        static halfedges_type::element_function_type m_halfedges_element;
        static halfedges_type::const_element_function_type m_halfedges_const_element;
    };

    template<typename Traits, typename M>
    std::ostream& operator<<(std::ostream& os, const Vertex<Traits, M>& vertex);

} // namespace quetzal::brep

//------------------------------------------------------------------------------
template<typename Traits, typename M>
quetzal::brep::Vertex<Traits, M>::Vertex() :
    Flags(),
    m_pmesh(nullptr),
    m_id(nullid),
    m_idHalfedge(nullid),
    m_halfedge_ids(),
    m_attributes(),
    m_properties(),
    m_halfedges(m_pmesh, nullid, m_halfedges_size, m_halfedges_first, m_halfedges_last, m_halfedges_end, m_halfedges_forward, m_halfedges_reverse, m_halfedges_element, m_halfedges_const_element)
{
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
quetzal::brep::Vertex<Traits, M>::Vertex(mesh_type& mesh, id_type id, id_type idHalfedge, const attributes_type& attributes, const Properties& properties) :
    Flags(),
    m_pmesh(&mesh),
    m_id(id),
    m_idHalfedge(idHalfedge),
    m_halfedge_ids(),
    m_attributes(attributes),
    m_properties(properties),
    m_halfedges(m_pmesh, id, m_halfedges_size, m_halfedges_first, m_halfedges_last, m_halfedges_end, m_halfedges_forward, m_halfedges_reverse, m_halfedges_element, m_halfedges_const_element)
{
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Vertex<Traits, M>::mesh_type& quetzal::brep::Vertex<Traits, M>::mesh()
{
    assert(m_pmesh != nullptr);
    return *m_pmesh;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
const typename quetzal::brep::Vertex<Traits, M>::mesh_type& quetzal::brep::Vertex<Traits, M>::mesh() const
{
    assert(m_pmesh != nullptr);
    return *m_pmesh;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
quetzal::id_type quetzal::brep::Vertex<Traits, M>::id() const
{
    return m_id;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
quetzal::id_type quetzal::brep::Vertex<Traits, M>::halfedge_id() const
{
    return m_idHalfedge;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Vertex<Traits, M>::set_halfedge_id(id_type id)
{
    m_idHalfedge = id;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
const typename quetzal::brep::Vertex<Traits, M>::halfedge_type& quetzal::brep::Vertex<Traits, M>::halfedge() const
{
    assert(m_pmesh != nullptr);
    return m_pmesh->halfedge(m_idHalfedge);
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Vertex<Traits, M>::halfedge_type& quetzal::brep::Vertex<Traits, M>::halfedge()
{
    assert(m_pmesh != nullptr);
    return m_pmesh->halfedge(m_idHalfedge);
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Vertex<Traits, M>::size_type quetzal::brep::Vertex<Traits, M>::halfedge_count() const
{
    return m_halfedges.size();
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
const typename quetzal::brep::Vertex<Traits, M>::halfedges_type& quetzal::brep::Vertex<Traits, M>::halfedges() const
{
    return m_halfedges;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Vertex<Traits, M>::halfedges_type& quetzal::brep::Vertex<Traits, M>::halfedges()
{
    return m_halfedges;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
const typename quetzal::brep::Vertex<Traits, M>::halfedge_type& quetzal::brep::Vertex<Traits, M>::halfedge(id_type id) const
{
    assert(m_halfedge_ids.contains(id));
    return m_pmesh->halfedge(id);
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Vertex<Traits, M>::halfedge_type& quetzal::brep::Vertex<Traits, M>::halfedge(id_type id)
{
    assert(m_halfedge_ids.contains(id));
    return m_pmesh->halfedge(id);
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
const typename quetzal::brep::Vertex<Traits, M>::halfedge_ids_type& quetzal::brep::Vertex<Traits, M>::halfedge_ids() const
{
    assert(false); // ...
    return m_halfedge_ids;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Vertex<Traits, M>::halfedge_ids_type& quetzal::brep::Vertex<Traits, M>::halfedge_ids()
{
    assert(false); // ...
    return m_halfedge_ids;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
const typename quetzal::brep::Vertex<Traits, M>::halfedge_ids_type& quetzal::brep::Vertex<Traits, M>::halfedge_ids(id_type surface_id) const
{
    assert(false); // ...
    return m_halfedge_ids;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Vertex<Traits, M>::halfedge_ids_type& quetzal::brep::Vertex<Traits, M>::halfedge_ids(id_type surface_id)
{
    assert(false); // ...
    return m_halfedge_ids;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
bool quetzal::brep::Vertex<Traits, M>::contains_halfedge(id_type id) const
{
    assert(false); // ...
    return m_halfedge_ids.contains(id);
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Vertex<Traits, M>::insert_halfedge(id_type id)
{
    assert(false); // ...
    assert(id != nullid);
    assert(!m_halfedge_ids.contains(id));
    m_halfedge_ids.insert(id);
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Vertex<Traits, M>::link_halfedge(id_type id)
{
    assert(false); // ...
    assert(m_pmesh != nullptr);
    m_pmesh->halfedge(id).set_vertex_id(m_id);
    insert_halfedge(id);
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Vertex<Traits, M>::unlink_halfedge(id_type id)
{
    assert(false); // ...
    assert(m_pmesh != nullptr);
    assert(id != nullid);
    assert(m_halfedge_ids.contains(id));

    m_halfedge_ids.erase(id);
    m_pmesh->halfedge(id).set_vertex_id(nullid);
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename const quetzal::brep::Vertex<Traits, M>::attributes_type& quetzal::brep::Vertex<Traits, M>::attributes() const
{
    return m_attributes;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Vertex<Traits, M>::attributes_type& quetzal::brep::Vertex<Traits, M>::attributes()
{
    return m_attributes;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Vertex<Traits, M>::set_attributes(const attributes_type& attributes)
{
    m_attributes = attributes;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename const quetzal::Properties& quetzal::brep::Vertex<Traits, M>::properties() const
{
    return m_properties;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::Properties& quetzal::brep::Vertex<Traits, M>::properties()
{
    return m_properties;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
bool quetzal::brep::Vertex<Traits, M>::check() const
{
    assert(m_pmesh != nullptr);

    if (deleted())
    {
        return true;
    }

    return m_id < m_pmesh->vertex_store_count() && m_pmesh->valid_halfedge_id(m_idHalfedge);
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Vertex<Traits, M>::set_mesh(mesh_type& mesh)
{
    m_pmesh = &mesh;
    m_halfedges.set_source(mesh);
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Vertex<Traits, M>::set_id(id_type id)
{
    m_id = id;
    m_halfedges.set_id(id);
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Vertex<Traits, M>::check_mesh(const mesh_type* const pmesh) const
{
    assert(m_pmesh == pmesh);
    assert(m_halfedges.check_source(pmesh));
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Vertex<Traits, M>::halfedges_type::size_function_type quetzal::brep::Vertex<Traits, M>::m_halfedges_size = [](const mesh_type& mesh, id_type id) -> size_t
{
    const auto& halfedges = mesh.vertex(id).halfedges();
    return std::distance(halfedges.begin(), halfedges.end());
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Vertex<Traits, M>::halfedges_type::terminal_function_type quetzal::brep::Vertex<Traits, M>::m_halfedges_first = [](const mesh_type& mesh, id_type id) -> id_type
{
    id_type orig_id = mesh.vertex(id).halfedge_id();
    id_type halfedge_id = orig_id;

    for (;;)
    {
        if (mesh.halfedge(halfedge_id).border())
        {
            break;
        }

        halfedge_id = mesh.halfedge(halfedge_id).partner().next_id();
        if (halfedge_id == orig_id) // There were no borders (but we had to iterate all the way around to determine this ...)
        {
            break;
        }
    }

    return halfedge_id;
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Vertex<Traits, M>::halfedges_type::terminal_function_type quetzal::brep::Vertex<Traits, M>::m_halfedges_last = [](const mesh_type& mesh, id_type id) -> id_type
{
    id_type orig_id = mesh.vertex(id).halfedge_id();
    id_type halfedge_id = orig_id;
    id_type prev_id = nullid;

    for (;;)
    {
        prev_id = halfedge_id;
        halfedge_id = mesh.halfedge(halfedge_id).prev().partner_id();
        if (halfedge_id == orig_id) // There were no borders (but we had to iterate all the way around to determine this ...)
        {
            break;
        }

        if (mesh.halfedge(halfedge_id).prev().border())
        {
            break;
        }
    }

    return prev_id;
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Vertex<Traits, M>::halfedges_type::terminal_function_type quetzal::brep::Vertex<Traits, M>::m_halfedges_end = [](const mesh_type& mesh, id_type id) -> id_type
{
    mesh;
    id;
    return nullid;
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Vertex<Traits, M>::halfedges_type::iterate_function_type quetzal::brep::Vertex<Traits, M>::m_halfedges_forward = [](const mesh_type& mesh, id_type id, id_type i) -> id_type
{
    if (mesh.halfedge(i).prev().border())
    {
        return nullid;
    }

	i = mesh.halfedge(i).prev().partner_id();
    if (i == m_halfedges_first(mesh, id))
    {
        return nullid;
    }

	return i;
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Vertex<Traits, M>::halfedges_type::iterate_function_type quetzal::brep::Vertex<Traits, M>::m_halfedges_reverse = [](const mesh_type& mesh, id_type id, id_type i) -> id_type
{
    if (mesh.halfedge(i).border())
    {
        return nullid;
    }

    i = mesh.halfedge(i).partner().next_id();
    if (i == m_halfedges_last(mesh, id))
    {
        return nullid;
    }

	return i;
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Vertex<Traits, M>::halfedges_type::element_function_type quetzal::brep::Vertex<Traits, M>::m_halfedges_element = [](mesh_type& mesh, id_type id, id_type i) -> quetzal::brep::Vertex<Traits, M>::halfedge_type&
{
    id;
    auto& halfedge = mesh.halfedge(i);
    return halfedge;
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Vertex<Traits, M>::halfedges_type::const_element_function_type quetzal::brep::Vertex<Traits, M>::m_halfedges_const_element = [](const mesh_type& mesh, id_type id, id_type i) -> const quetzal::brep::Vertex<Traits, M>::halfedge_type&
{
    id;
    const auto& halfedge = mesh.halfedge(i);
    return halfedge;
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
std::ostream& quetzal::brep::operator<<(std::ostream& os, const Vertex<Traits, M>& vertex)
{
    os << static_cast<Flags>(vertex);
    os << '\t' << vertex.id();

    if (!vertex.deleted())
    {
        os << '\t' << vertex.halfedge_id();
        if (vertex.halfedge().border())
        {
            os << "\tborder";
        }
        else
        {
            os << '\t' << vertex.halfedge_count() << " edges";
        }

        os << '\t' << vertex.attributes();
        os << '\t' << vertex.properties();
    }

    os << '\n';
    return os;
}

#endif // QUETZAL_BREP_VERTEX_HPP
