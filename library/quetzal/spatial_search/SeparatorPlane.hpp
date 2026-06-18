#if !defined(QUETZAL_SPATIAL_SEARCH_SEPARATORPLANE_HPP)
#define QUETZAL_SPATIAL_SEARCH_SEPARATORPLANE_HPP
//------------------------------------------------------------------------------
// spatial_search
// SeparatorPlane.hpp
//------------------------------------------------------------------------------

#include <iostream>
#include <tuple>

namespace quetzal::spatial_search
{

    //--------------------------------------------------------------------------
    template<typename T>
    class SeparatorPlane
    {
    public:

        SeparatorPlane();
        SeparatorPlane(size_t nComponent, T value);

        SeparatorPlane(const SeparatorPlane&) = default;
        SeparatorPlane(SeparatorPlane&&) = default;
        ~SeparatorPlane() = default;

        SeparatorPlane& operator=(const SeparatorPlane&) = default;
        SeparatorPlane& operator=(SeparatorPlane&&) = default;

        operator std::tuple<size_t, T>() const;

        void set(size_t nComponent, T value);

        size_t component() const;
        void set_component(size_t nComponent);

        T value() const;
        void set_value(T value);

    private:

        size_t m_nComponent;
        T m_value;
    };

    template<typename T>
    std::ostream& operator<<(std::ostream& os, const SeparatorPlane<T>& sp);

} // namespace quetzal::spatial_search

//------------------------------------------------------------------------------
template<typename T>
quetzal::spatial_search::SeparatorPlane<T>::SeparatorPlane() :
    SeparatorPlane(0, T(0))
{
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::spatial_search::SeparatorPlane<T>::SeparatorPlane(size_t nComponent, T value) :
    m_nComponent(nComponent),
    m_value(value)
{
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::spatial_search::SeparatorPlane<T>::operator std::tuple<size_t, T>() const
{
    return {m_nComponent, m_value};
}

//------------------------------------------------------------------------------
template<typename T>
void quetzal::spatial_search::SeparatorPlane<T>::set(size_t nComponent, T value)
{
    m_nComponent = nComponent;
    m_value = value;
    return;
}

//------------------------------------------------------------------------------
template<typename T>
size_t quetzal::spatial_search::SeparatorPlane<T>::component() const
{
    return m_nComponent;
}

//------------------------------------------------------------------------------
template<typename T>
void quetzal::spatial_search::SeparatorPlane<T>::set_component(size_t nComponent)
{
    m_nComponent = nComponent;
    return;
}

//------------------------------------------------------------------------------
template<typename T>
T quetzal::spatial_search::SeparatorPlane<T>::value() const
{
    return m_value;
}

//------------------------------------------------------------------------------
template<typename T>
void quetzal::spatial_search::SeparatorPlane<T>::set_value(T value)
{
    m_value = value;
    return;
}

//------------------------------------------------------------------------------
template<typename T>
std::ostream& quetzal::spatial_search::operator<<(std::ostream& os, const SeparatorPlane<T>& sp)
{
    os << "{[" << sp.component() << "]: " << sp.value() << "}";
    return os;
}

#endif // QUETZAL_SPATIAL_SEARCH_SEPARATORPLANE_HPP
