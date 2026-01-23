#if !defined(QUETZAL_BREP_HOLE_HPP)
#define QUETZAL_BREP_HOLE_HPP
//------------------------------------------------------------------------------
// brep
// Hole.hpp
//------------------------------------------------------------------------------

#include "id.hpp"
#include "quetzal/common/ElementsDirect.hpp"
#include "quetzal/common/Flags.hpp"
#include "quetzal/common/Properties.hpp"
#include <ostream>
#include <ranges>
#include <vector>
#include <cassert>

namespace quetzal::brep
{

    template<typename Traits>
    class Mesh;

    //--------------------------------------------------------------------------
    template<typename Traits, typename M = Mesh<Traits>>
    class Hole : public Flags
    {
    public:

        using mesh_type = M;
        using face_type = mesh_type::face_type;
        using halfedge_type = mesh_type::halfedge_type;
        using halfedges_type = ElementsDirect<Hole, halfedge_type, id_type>;
        using size_type = mesh_type::size_type;

        Hole();
        Hole(mesh_type& mesh, id_type halfedge_id);
        Hole(const Hole&) = default;
        Hole(Hole&&) noexcept = default;
        ~Hole() = default;

        Hole& operator=(const Hole&) = default;
        Hole& operator=(Hole&&) = default;

        const mesh_type& mesh() const;
        mesh_type& mesh();

        id_type face_id() const;
        void set_face_id(id_type face_id); // split_face operations can change the face

        const face_type& face() const;
        face_type& face();

        id_type halfedge_id() const;
        void set_halfedge_id(id_type halfedge_id);

        const halfedge_type& halfedge() const;
        halfedge_type& halfedge();

        // Virtual container interface, iterators for halfedges that make up this hole
        size_type halfedge_count() const;
        const halfedges_type& halfedges() const;
        halfedges_type& halfedges();

        std::vector<quetzal::id_type> halfedge_ids() const;

        const Properties& properties() const;
        Properties& properties();

        void reset_all() const override; // Flags

        bool check() const;

        // Internal use
        void set_mesh(mesh_type& mesh);
        void check_mesh(const mesh_type* const pmesh) const;

    private:

        mesh_type* m_pmesh;
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
    std::ostream& operator<<(std::ostream& os, const Hole<Traits, M>& hole);

} // namespace quetzal::brep

//------------------------------------------------------------------------------
template<typename Traits, typename M>
quetzal::brep::Hole<Traits, M>::Hole() :
    Flags(),
    m_pmesh(nullptr),
    m_halfedge_id(nullid),
    m_halfedges(m_halfedges_size, m_halfedges_first, m_halfedges_last, m_halfedges_end, m_halfedges_forward, m_halfedges_reverse, m_halfedges_element, m_halfedges_const_element),
    m_properties()
{
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
quetzal::brep::Hole<Traits, M>::Hole(mesh_type& mesh, id_type halfedge_id) :
    Flags(),
    m_pmesh(&mesh),
    m_halfedge_id(halfedge_id),
    m_halfedges(m_halfedges_size, m_halfedges_first, m_halfedges_last, m_halfedges_end, m_halfedges_forward, m_halfedges_reverse, m_halfedges_element, m_halfedges_const_element),
    m_properties()
{
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
const typename quetzal::brep::Hole<Traits, M>::mesh_type& quetzal::brep::Hole<Traits, M>::mesh() const
{
    assert(m_pmesh != nullptr);
    return *m_pmesh;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Hole<Traits, M>::mesh_type& quetzal::brep::Hole<Traits, M>::mesh()
{
    assert(m_pmesh != nullptr);
    return *m_pmesh;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
quetzal::id_type quetzal::brep::Hole<Traits, M>::face_id() const
{
    assert(m_pmesh != nullptr);
    return m_pmesh->halfedge(m_halfedge_id).face_id();
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Hole<Traits, M>::set_face_id(id_type face_id)
{
    // Assign hole halfedges to the new face
    for (auto& halfedge : halfedges())
    {
        halfedge.set_face_id(face_id);
    }
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
const typename quetzal::brep::Hole<Traits, M>::face_type& quetzal::brep::Hole<Traits, M>::face() const
{
    assert(m_pmesh != nullptr);
    return m_pmesh->halfedge(m_halfedge_id).face();
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Hole<Traits, M>::face_type& quetzal::brep::Hole<Traits, M>::face()
{
    assert(m_pmesh != nullptr);
    return m_pmesh->halfedge(m_halfedge_id).face();
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
quetzal::id_type quetzal::brep::Hole<Traits, M>::halfedge_id() const
{
    return m_halfedge_id;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Hole<Traits, M>::set_halfedge_id(id_type halfedge_id)
{
    m_halfedge_id = halfedge_id;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
const typename quetzal::brep::Hole<Traits, M>::halfedge_type& quetzal::brep::Hole<Traits, M>::halfedge() const
{
    assert(m_pmesh != nullptr);
    return m_pmesh->halfedge(m_halfedge_id);
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Hole<Traits, M>::halfedge_type& quetzal::brep::Hole<Traits, M>::halfedge()
{
    assert(m_pmesh != nullptr);
    return m_pmesh->halfedge(m_halfedge_id);
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Hole<Traits, M>::size_type quetzal::brep::Hole<Traits, M>::halfedge_count() const
{
    return halfedges().size();
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
const typename quetzal::brep::Hole<Traits, M>::halfedges_type& quetzal::brep::Hole<Traits, M>::halfedges() const
{
    m_halfedges.set_source(*this);
    return m_halfedges;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Hole<Traits, M>::halfedges_type& quetzal::brep::Hole<Traits, M>::halfedges()
{
    m_halfedges.set_source(*this);
    return m_halfedges;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
std::vector<quetzal::id_type> quetzal::brep::Hole<Traits, M>::halfedge_ids() const
{
    m_halfedges.set_source(*this);
    return std::ranges::transform_view(m_halfedges, [](const auto& halfedge) {
        return halfedge.id();
    }) | std::ranges::to<std::vector>();
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename const quetzal::Properties& quetzal::brep::Hole<Traits, M>::properties() const
{
    return m_properties;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::Properties& quetzal::brep::Hole<Traits, M>::properties()
{
    return m_properties;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Hole<Traits, M>::reset_all() const
{
    m_halfedges.set_source(*this);
    for (auto& halfedge : m_halfedges)
    {
        halfedge.reset();
        halfedge.vertex().reset();
    }

    Flags::reset();
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
bool quetzal::brep::Hole<Traits, M>::check() const
{
    if (deleted())
    {
        return true;
    }

    assert(m_pmesh != nullptr);
    return m_pmesh->valid_halfedge_id(m_halfedge_id);
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Hole<Traits, M>::set_mesh(mesh_type& mesh)
{
    m_pmesh = &mesh;
    m_halfedges.set_source(*this);
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
void quetzal::brep::Hole<Traits, M>::check_mesh(const mesh_type* const pmesh) const
{
    assert(m_pmesh == pmesh);
//    assert(m_halfedges.source() == pmesh); // ...
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Hole<Traits, M>::halfedges_type::size_function_type quetzal::brep::Hole<Traits, M>::m_halfedges_size = [](const Hole<Traits, M>& hole) -> size_t
{
    size_t n = 0;
    for (const auto& halfedge : hole.halfedges())
    {
        halfedge;
        ++n;
    }

    return n;
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Hole<Traits, M>::halfedges_type::terminal_function_type quetzal::brep::Hole<Traits, M>::m_halfedges_first = [](const Hole<Traits, M>& hole) -> id_type
{
    return hole.halfedge_id();
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Hole<Traits, M>::halfedges_type::terminal_function_type quetzal::brep::Hole<Traits, M>::m_halfedges_last = [](const Hole<Traits, M>& hole) -> id_type
{
    return hole.halfedge().prev_id();
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Hole<Traits, M>::halfedges_type::terminal_function_type quetzal::brep::Hole<Traits, M>::m_halfedges_end = [](const Hole<Traits, M>& hole) -> id_type
{
    hole;
    return nullid;
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Hole<Traits, M>::halfedges_type::iterate_function_type quetzal::brep::Hole<Traits, M>::m_halfedges_forward = [](const Hole<Traits, M>& hole, id_type i) -> id_type
{
    assert(i != m_halfedges_end(hole));
    i = hole.mesh().halfedge_store()[i].next_id();
    return i != m_halfedges_first(hole) ? i : nullid;
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Hole<Traits, M>::halfedges_type::iterate_function_type quetzal::brep::Hole<Traits, M>::m_halfedges_reverse = [](const Hole<Traits, M>& hole, id_type i) -> id_type
{
    assert(i != m_halfedges_end(hole));
    i = hole.mesh().halfedge_store()[i].prev_id();
    return i != m_halfedges_last(hole) ? i : nullid;
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Hole<Traits, M>::halfedges_type::element_function_type quetzal::brep::Hole<Traits, M>::m_halfedges_element = [](Hole<Traits, M>& hole, id_type i) -> quetzal::brep::Hole<Traits, M>::halfedge_type&
{
    return hole.mesh().halfedge(i);
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
typename quetzal::brep::Hole<Traits, M>::halfedges_type::const_element_function_type quetzal::brep::Hole<Traits, M>::m_halfedges_const_element = [](const Hole<Traits, M>& hole, id_type i) -> const quetzal::brep::Hole<Traits, M>::halfedge_type&
{
    return hole.mesh().halfedge(i);
};

//------------------------------------------------------------------------------
template<typename Traits, typename M>
std::ostream& quetzal::brep::operator<<(std::ostream& os, const Hole<Traits, M>& hole)
{
    os << static_cast<Flags>(hole);

    if (!hole.deleted())
    {
        os << '\t' << hole.halfedge_count() << " edges";
        os << "\tf " << hole.face_id();
        os << "\th " << hole.halfedge_id();
        os << '\t' << hole.properties();
    }

    os << '\n';
    return os;
}

#endif // QUETZAL_BREP_HOLE_HPP
