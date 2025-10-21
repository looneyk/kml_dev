#if !defined(QUETZAL_ELEMENTSDIRECTCONSTITERATOR_HPP)
#define QUETZAL_ELEMENTSDIRECTCONSTITERATOR_HPP
//------------------------------------------------------------------------------
// common
// ElementsDirectConstIterator.hpp
//------------------------------------------------------------------------------

#include "id.hpp"
#include <iterator>
#include <functional>
#include <cassert>

namespace quetzal
{

    //--------------------------------------------------------------------------
    template<typename S, typename E, typename I>
    class ElementsDirectConstIterator
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
        using iterate_function_type = std::function<i_type(const source_type&, i_type)>; // returns the next i given the current i
        using element_function_type = std::function<const value_type&(const source_type&, i_type)>; // returns a reference to the element at i

        ElementsDirectConstIterator() = default; // to satisfy iterator constraints, don't use it this way ...
        ElementsDirectConstIterator(const source_type& source, i_type end); // end
        ElementsDirectConstIterator(const source_type& source, i_type begin, i_type end, iterate_function_type advance, element_function_type element);
        explicit ElementsDirectConstIterator(const ElementsDirectIterator<S, E, I>& other);
        ElementsDirectConstIterator(const ElementsDirectConstIterator&) = default;
        ElementsDirectConstIterator(ElementsDirectConstIterator&&) = default;
        ~ElementsDirectConstIterator() = default;

        ElementsDirectConstIterator& operator=(const ElementsDirectConstIterator&) = default;
        ElementsDirectConstIterator& operator=(ElementsDirectConstIterator&&) = default;

        ElementsDirectConstIterator& operator++();
        ElementsDirectConstIterator operator++(int);

        reference operator*() const;
        pointer operator->() const;

        friend bool operator==(const ElementsDirectConstIterator<S, E, I>& i, const ElementsDirectConstIterator<S, E, I>& j)
        {
            assert(i.m_psource == j.m_psource);
            return i.m_i == j.m_i;
        };

        friend bool operator!=(const ElementsDirectConstIterator<S, E, I>& i, const ElementsDirectConstIterator<S, E, I>& j)
        {
            return !(i == j);
        };

    private:

        const source_type* m_psource;
        i_type m_begin;
        i_type m_end;
        i_type m_i;
        iterate_function_type m_advance;
        element_function_type m_element;
    };

} // namespace quetzal

//------------------------------------------------------------------------------
template<typename S, typename E, typename I>
quetzal::ElementsDirectConstIterator<S, E, I>::ElementsDirectConstIterator(const source_type& source, i_type end) :
    m_psource(&source),
    m_begin(end),
    m_end(end),
    m_i(end),
    m_advance(),
    m_element()
{
}

//------------------------------------------------------------------------------
template<typename S, typename E, typename I>
quetzal::ElementsDirectConstIterator<S, E, I>::ElementsDirectConstIterator(const source_type& source, i_type begin, i_type end, iterate_function_type advance, element_function_type element) :
    m_psource(&source),
    m_begin(begin),
    m_end(end),
    m_i(begin),
    m_advance(advance),
    m_element(element)
{
}

//------------------------------------------------------------------------------
template<typename S, typename E, typename I>
quetzal::ElementsDirectConstIterator<S, E, I>::ElementsDirectConstIterator(const ElementsDirectIterator<S, E, I>& other) :
    m_psource(other.m_psource),
    m_begin(other.m_begin),
    m_end(other.m_end),
    m_i(other.m_i),
    m_advance(other.m_advance),
    m_element(other.m_element)
{
}

//------------------------------------------------------------------------------
template<typename S, typename E, typename I>
quetzal::ElementsDirectConstIterator<S, E, I>& quetzal::ElementsDirectConstIterator<S, E, I>::operator++()
{
    assert(m_psource != nullptr);
    m_i = m_advance(*m_psource, m_i);
    return *this;
}

//------------------------------------------------------------------------------
template<typename S, typename E, typename I>
quetzal::ElementsDirectConstIterator<S, E, I> quetzal::ElementsDirectConstIterator<S, E, I>::operator++(int)
{
    ElementsDirectConstIterator i(*this);
    operator++();
    return i;
}

//------------------------------------------------------------------------------
template<typename S, typename E, typename I>
typename quetzal::ElementsDirectConstIterator<S, E, I>::reference quetzal::ElementsDirectConstIterator<S, E, I>::operator*() const
{
    assert(m_psource != nullptr);
    return m_element(*m_psource, m_i);
}

//------------------------------------------------------------------------------
template<typename S, typename E, typename I>
typename quetzal::ElementsDirectConstIterator<S, E, I>::pointer quetzal::ElementsDirectConstIterator<S, E, I>::operator->() const
{
    assert(m_psource != nullptr);
    return &m_element(*m_psource, m_i);
}

#endif // QUETZAL_ELEMENTSDIRECTCONSTITERATOR_HPP
