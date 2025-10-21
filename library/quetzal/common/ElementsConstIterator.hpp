#if !defined(QUETZAL_ELEMENTSCONSTITERATOR_HPP)
#define QUETZAL_ELEMENTSCONSTITERATOR_HPP
//------------------------------------------------------------------------------
// common
// ElementsConstIterator.hpp
//------------------------------------------------------------------------------

#include "id.hpp"
#include <iterator>
#include <functional>
#include <cassert>

namespace quetzal
{

    //--------------------------------------------------------------------------
    template<typename S, typename E, typename I>
    class ElementsConstIterator
    {
    public:

        using source_type = S;
        using i_type = I;
        using iterator_category = std::forward_iterator_tag;
        using value_type = const E;
        using pointer = const E*;
        using reference = const E&;
        using difference_type = ptrdiff_t;
        using size_type = size_t;
        using iterate_function_type = std::function<i_type(const source_type&, id_type, i_type)>; // returns the next i given the current i
        using element_function_type = std::function<const value_type&(const source_type&, id_type, i_type)>; // returns a reference to the element at i

        ElementsConstIterator() = default; // to satisfy iterator constraints, don't use it this way ...
        ElementsConstIterator(const source_type& source, id_type id, i_type end); // end
        ElementsConstIterator(const source_type& source, id_type id, i_type begin, i_type end, iterate_function_type advance, element_function_type element);
        explicit ElementsConstIterator(const ElementsIterator<S, E, I>& other);
        ElementsConstIterator(const ElementsConstIterator&) = default;
        ElementsConstIterator(ElementsConstIterator&&) = default;
        ~ElementsConstIterator() = default;

        ElementsConstIterator& operator=(const ElementsConstIterator&) = default;
        ElementsConstIterator& operator=(ElementsConstIterator&&) = default;

        ElementsConstIterator& operator++();
        ElementsConstIterator operator++(int);

        reference operator*() const;
        pointer operator->() const;

        friend bool operator==(const ElementsConstIterator<S, E, I>& i, const ElementsConstIterator<S, E, I>& j)
        {
            assert(i.m_psource == j.m_psource && i.m_id == j.m_id);
            return i.m_i == j.m_i;
        };

        friend bool operator!=(const ElementsConstIterator<S, E, I>& i, const ElementsConstIterator<S, E, I>& j)
        {
            return !(i == j);
        };

    private:

        const source_type* m_psource;
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
quetzal::ElementsConstIterator<S, E, I>::ElementsConstIterator(const source_type& source, id_type id, i_type end) :
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
quetzal::ElementsConstIterator<S, E, I>::ElementsConstIterator(const source_type& source, id_type id, i_type begin, i_type end, iterate_function_type advance, element_function_type element) :
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
quetzal::ElementsConstIterator<S, E, I>::ElementsConstIterator(const ElementsIterator<S, E, I>& other) :
    m_psource(other.m_psource),
    m_id(other.m_id),
    m_begin(other.m_begin),
    m_end(other.m_end),
    m_i(other.m_i),
    m_advance(other.m_advance),
    m_element(other.m_element)
{
}

//------------------------------------------------------------------------------
template<typename S, typename E, typename I>
quetzal::ElementsConstIterator<S, E, I>& quetzal::ElementsConstIterator<S, E, I>::operator++()
{
    assert(m_psource != nullptr);
    m_i = m_advance(*m_psource, m_id, m_i);
    return *this;
}

//------------------------------------------------------------------------------
template<typename S, typename E, typename I>
quetzal::ElementsConstIterator<S, E, I> quetzal::ElementsConstIterator<S, E, I>::operator++(int)
{
    ElementsConstIterator i(*this);
    operator++();
    return i;
}

//------------------------------------------------------------------------------
template<typename S, typename E, typename I>
typename quetzal::ElementsConstIterator<S, E, I>::reference quetzal::ElementsConstIterator<S, E, I>::operator*() const
{
    assert(m_psource != nullptr);
    return m_element(*m_psource, m_id, m_i);
}

//------------------------------------------------------------------------------
template<typename S, typename E, typename I>
typename quetzal::ElementsConstIterator<S, E, I>::pointer quetzal::ElementsConstIterator<S, E, I>::operator->() const
{
    assert(m_psource != nullptr);
    return &m_element(*m_psource, m_id, m_i);
}

#endif // QUETZAL_ELEMENTSCONSTITERATOR_HPP
