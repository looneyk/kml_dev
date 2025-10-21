#if !defined(QUETZAL_ELEMENTSITERATOR_HPP)
#define QUETZAL_ELEMENTSITERATOR_HPP
//------------------------------------------------------------------------------
// common
// ElementsIterator.hpp
//------------------------------------------------------------------------------

#include "id.hpp"
#include <iterator>
#include <functional>
#include <cassert>

namespace quetzal
{

    template<typename S, typename E, typename I>
    class ElementsConstIterator;

    //--------------------------------------------------------------------------
    template<typename S, typename E, typename I>
    class ElementsIterator
    {
    public:

        using source_type = S;
        using i_type = I;
        using iterator_category = std::forward_iterator_tag;
        using value_type = E;
        using pointer = E*;
        using reference = E&;
        using difference_type = ptrdiff_t;
        using size_type = size_t;
        using iterate_function_type = std::function<i_type(source_type&, id_type, i_type)>; // returns the next i given the current i
        using element_function_type = std::function<value_type&(source_type&, id_type, i_type)>; // returns a reference to the element at i

        ElementsIterator() = default; // to satisfy iterator constraints, don't use it this way ...
        ElementsIterator(source_type& source, id_type id, i_type end); // end
        ElementsIterator(source_type& source, id_type id, i_type begin, i_type end, iterate_function_type advance, element_function_type element);
        ElementsIterator(const ElementsIterator&) = default;
        ElementsIterator(ElementsIterator&&) = default;
        ~ElementsIterator() = default;

        ElementsIterator& operator=(const ElementsIterator&) = default;
        ElementsIterator& operator=(ElementsIterator&&) = default;

        ElementsIterator& operator++();
        ElementsIterator operator++(int);

        reference operator*() const;
        pointer operator->() const;

        friend bool operator==(const ElementsIterator<S, E, I>& i, const ElementsIterator<S, E, I>& j)
        {
            assert(i.m_psource == j.m_psource && i.m_id == j.m_id);
            return i.m_i == j.m_i;
        };

        friend bool operator!=(const ElementsIterator<S, E, I>& i, const ElementsIterator<S, E, I>& j)
        {
            return !(i == j);
        };

    private:

        template<typename S, typename E, typename I>
        friend class ElementsConstIterator;

        source_type* m_psource;
        id_type m_id;
        i_type m_begin;
        i_type m_end;
        i_type m_i;
        iterate_function_type m_advance;
        element_function_type m_element;
    };

} // namespace quetzal

//------------------------------------------------------------------------------
template<typename S, typename E, typename I>
quetzal::ElementsIterator<S, E, I>::ElementsIterator(source_type& source, id_type id, i_type end) :
    m_psource(&source),
    m_id(id),
    m_begin(end),
    m_end(end),
    m_i(end),
    m_advance(),
    m_element()
{
}

//------------------------------------------------------------------------------
template<typename S, typename E, typename I>
quetzal::ElementsIterator<S, E, I>::ElementsIterator(source_type& source, id_type id, i_type begin, i_type end, iterate_function_type advance, element_function_type element) :
    m_psource(&source),
    m_id(id),
    m_begin(begin),
    m_end(end),
    m_i(begin),
    m_advance(advance),
    m_element(element)
{
}

//------------------------------------------------------------------------------
template<typename S, typename E, typename I>
quetzal::ElementsIterator<S, E, I>& quetzal::ElementsIterator<S, E, I>::operator++()
{
    assert(m_psource != nullptr);
    m_i = m_advance(*m_psource, m_id, m_i);
    return *this;
}

//------------------------------------------------------------------------------
template<typename S, typename E, typename I>
quetzal::ElementsIterator<S, E, I> quetzal::ElementsIterator<S, E, I>::operator++(int)
{
    ElementsIterator i(*this);
    operator++();
    return i;
}

//------------------------------------------------------------------------------
template<typename S, typename E, typename I>
typename quetzal::ElementsIterator<S, E, I>::reference quetzal::ElementsIterator<S, E, I>::operator*() const
{
    assert(m_psource != nullptr);
    return m_element(*m_psource, m_id, m_i);
}

//------------------------------------------------------------------------------
template<typename S, typename E, typename I>
typename quetzal::ElementsIterator<S, E, I>::pointer quetzal::ElementsIterator<S, E, I>::operator->() const
{
    assert(m_psource != nullptr);
    return &m_element(*m_psource, m_id, m_i);
}

#endif // QUETZAL_ELEMENTSITERATOR_HPP
