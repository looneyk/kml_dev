#if !defined(QUETZAL_BREP_SEAM_HPP)
#define QUETZAL_BREP_SEAM_HPP
//------------------------------------------------------------------------------
// brep
// Seam.hpp
//------------------------------------------------------------------------------

#include "id.hpp"
#include "quetzal/common/Elements.hpp"
#include "quetzal/common/Flags.hpp"
#include "quetzal/common/Properties.hpp"
#include <ostream>
#include <vector>
#include <cassert>

namespace quetzal::brep
{

    template<typename Traits>
    class Mesh;

    //--------------------------------------------------------------------------
    template<typename Traits, typename M = Mesh<Traits>>
    class Seam : public Flags
    {
    public:

        using mesh_type = M;
        using surface_type = mesh_type::surface_type;
        using halfedge_type = mesh_type::halfedge_type;
        using halfedges_type = Elements<mesh_type, halfedge_type, id_type>;
        using size_type = mesh_type::size_type;

        Seam();
        Seam(mesh_type& mesh, id_type surface_id, id_type halfedge_id);
        Seam(const Seam&) = default;
        Seam(Seam&&) noexcept = default;
        ~Seam() = default;

        Seam& operator=(const Seam&) = default;
        Seam& operator=(Seam&&) = default;

        const mesh_type& mesh() const;
        mesh_type& mesh();

        id_type surface_id() const;
        void set_surface_id(id_type surface_id);

        const surface_type& surface() const;
        surface_type& surface();

        // Virtual container interface, iterators for halfedges that make up this seam
        size_type halfedge_count() const;
        const halfedges_type& halfedges() const;
        halfedges_type& halfedges();

        std::vector<id_type> find_seam_sections(id_type partner_surface_id);
        std::vector<id_type> find_seam_sections(const std::string& partner_surface_name);

        const Properties& properties() const;
        Properties& properties();

        // Internal use
        void set_mesh(mesh_type& mesh);
        void check_mesh(const mesh_type* const pmesh) const;

        id_type halfedge_id() const;

    private:

        mesh_type* m_pmesh;
        id_type m_surface_id;
        id_type m_halfedge_id;
        halfedges_type m_halfedges;
        Properties m_properties;

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
    std::ostream& operator<<(std::ostream& os, const Seam<Traits, M>& seam);

} // namespace quetzal::brep

//------------------------------------------------------------------------------
template<typename Traits, typename M>
quetzal::brep::Seam<Traits, M>::Seam() :
    Flags(),
    m_pmesh(nullptr),
    m_surface_id(nullid),
    m_halfedge_id(nullid),
    m_halfedges(m_pmesh, nullid, m_halfedges_size, m_halfedges_first, m_halfedges_last, m_halfedges_end, m_halfedges_forward, m_halfedges_reverse, m_halfedges_element, m_halfedges_const_element),
    m_properties()
{
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
quetzal::brep::Seam<Traits, M>::Seam(mesh_type& mesh, id_type surface_id, id_type halfedge_id) :
    Flags(),
    m_pmesh(&mesh),
    m_surface_id(surface_id),
    m_halfedge_id(halfedge_id),
    m_halfedges(m_pmesh, halfedge_id, m_halfedges_size, m_halfedges_first, m_halfedges_last, m_halfedges_end, m_halfedges_forward, m_halfedges_reverse, m_halfedges_element, m_halfedges_const_element),
    m_properties()
{
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
const typename quetzal::brep::Seam<Traits, M>::mesh_type& quetzal::brep::Seam<Traits, M>::mesh() const
{
    assert(m_pmesh != nullptr);
    return *m_pmesh;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Seam<Traits, M>::mesh_type& quetzal::brep::Seam<Traits, M>::mesh()
{
    assert(m_pmesh != nullptr);
    return *m_pmesh;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
quetzal::id_type quetzal::brep::Seam<Traits, M>::surface_id() const
{
    return m_surface_id;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Seam<Traits, M>::set_surface_id(id_type surface_id)
{
    m_surface_id = surface_id;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
const typename quetzal::brep::Seam<Traits, M>::surface_type& quetzal::brep::Seam<Traits, M>::surface() const
{
    assert(m_pmesh != nullptr);
    return m_pmesh->surface(m_surface_id);
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Seam<Traits, M>::surface_type& quetzal::brep::Seam<Traits, M>::surface()
{
    assert(m_pmesh != nullptr);
    return m_pmesh->surface(m_surface_id);
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Seam<Traits, M>::size_type quetzal::brep::Seam<Traits, M>::halfedge_count() const
{
    surface().check_regenerate_seams();
    return m_halfedges.size();
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
const typename quetzal::brep::Seam<Traits, M>::halfedges_type& quetzal::brep::Seam<Traits, M>::halfedges() const
{
    surface().check_regenerate_seams();
    return m_halfedges;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Seam<Traits, M>::halfedges_type& quetzal::brep::Seam<Traits, M>::halfedges()
{
    surface().check_regenerate_seams();
    return m_halfedges;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
std::vector<quetzal::id_type> quetzal::brep::Seam<Traits, M>::find_seam_sections(id_type partner_surface_id)
{
    surface().check_regenerate_seams();
    std::vector<id_type> ids;

    bool found = false;

    for (const auto& halfedge : m_halfedges)
    {
        if (halfedge.partner_surface_id() == partner_surface_id)
        {
            if (found)
            {
                continue;
            }

            ids.push_back(halfedge.id());
            found = true;
        }
        else
        {
            found = false;
        }
    }

    return ids;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
std::vector<quetzal::id_type> quetzal::brep::Seam<Traits, M>::find_seam_sections(const std::string& partner_surface_name)
{
    surface().check_regenerate_seams();
    std::vector<id_type> ids;

    bool found = false;

    for (const auto& halfedge : m_halfedges)
    {
        if (!halfedge.border() && m_pmesh->surface(halfedge.partner_surface_id()).name() == partner_surface_name)
        {
            if (found)
            {
                continue;
            }

            ids.push_back(halfedge.id());
            found = true;
        }
        else
        {
            found = false;
        }
    }

    return ids;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
const typename quetzal::Properties& quetzal::brep::Seam<Traits, M>::properties() const
{
    return m_properties;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::Properties& quetzal::brep::Seam<Traits, M>::properties()
{
    return m_properties;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Seam<Traits, M>::set_mesh(mesh_type& mesh)
{
    m_pmesh = &mesh;
    m_halfedges.set_source(mesh);
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Seam<Traits, M>::check_mesh(const mesh_type* const pmesh) const
{
    assert(m_pmesh == pmesh);
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
quetzal::id_type quetzal::brep::Seam<Traits, M>::halfedge_id() const
{
    surface().check_regenerate_seams();
    return m_halfedge_id;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Seam<Traits, M>::halfedges_type::size_function_type quetzal::brep::Seam<Traits, M>::m_halfedges_size = [](const mesh_type& mesh, id_type id) -> size_t
{
    id_type id0 = id;
    size_t n = 0;
    for_each_halfedge(id0, [&mesh, &n](id_type id) -> id_type
    {
        ++n;
        return mesh.halfedge(id).next_id();
    });
    return n;
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Seam<Traits, M>::halfedges_type::terminal_function_type quetzal::brep::Seam<Traits, M>::m_halfedges_first = [](const mesh_type& mesh, id_type id) -> id_type
{
    mesh;
    return id;
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Seam<Traits, M>::halfedges_type::terminal_function_type quetzal::brep::Seam<Traits, M>::m_halfedges_last = [](const mesh_type& mesh, id_type id) -> id_type
{
    return prev_surface_halfedge_id(mesh, id);
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Seam<Traits, M>::halfedges_type::terminal_function_type quetzal::brep::Seam<Traits, M>::m_halfedges_end = [](const mesh_type& mesh, id_type id) -> id_type
{
    mesh;
    id;
    return nullid;
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Seam<Traits, M>::halfedges_type::iterate_function_type quetzal::brep::Seam<Traits, M>::m_halfedges_forward = [](const mesh_type& mesh, id_type id, id_type i) -> id_type
{
    assert(i != m_halfedges_end(mesh, id));
    i = next_surface_halfedge_id(mesh, i);
    return i != m_halfedges_first(mesh, id) ? i : nullid;
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Seam<Traits, M>::halfedges_type::iterate_function_type quetzal::brep::Seam<Traits, M>::m_halfedges_reverse = [](const mesh_type& mesh, id_type id, id_type i) -> id_type
{
    assert(i != m_halfedges_end(mesh, id));
    i = prev_surface_halfedge_id(mesh, i);
    return i != m_halfedges_last(mesh, id) ? i : nullid;
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Seam<Traits, M>::halfedges_type::element_function_type quetzal::brep::Seam<Traits, M>::m_halfedges_element = [](mesh_type& mesh, id_type id, id_type i) -> quetzal::brep::Seam<Traits, M>::halfedge_type&
{
    id;
    return mesh.halfedge(i);
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Seam<Traits, M>::halfedges_type::const_element_function_type quetzal::brep::Seam<Traits, M>::m_halfedges_const_element = [](const mesh_type& mesh, id_type id, id_type i) -> const quetzal::brep::Seam<Traits, M>::halfedge_type&
{
    id;
    return mesh.halfedge(i);
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
std::ostream& quetzal::brep::operator<<(std::ostream& os, const Seam<Traits, M>& seam)
{
    os << static_cast<Flags>(seam);

    if (!seam.deleted())
    {
        os << '\t' << seam.halfedge_count() << " edges";
        os << "\ts " << seam.surface_id();
        os << "\th " << seam.halfedge_id();
        os << '\t' << seam.properties();
    }

    os << '\n';
    return os;
}

#endif // QUETZAL_BREP_SEAM_HPP
