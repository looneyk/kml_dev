#if !defined(QUETZAL_STRATUM_VIEW_BUFFER_HPP)
#define QUETZAL_STRATUM_VIEW_BUFFER_HPP
//------------------------------------------------------------------------------
// stratum_view
// Buffer.hpp
//------------------------------------------------------------------------------

#include "types.hpp"
#include <algorithm>
#include <concepts>
#include <string>
#include <vector>
#include <cassert>

namespace quetzal::stratum_view
{

    //--------------------------------------------------------------------------
    // A grid of cells to be drawn. Row 0 is the lowest, as in the simulation
    template<typename T>
    class Buffer
    {
    public:

        using value_type = T;

        Buffer() = default;
        Buffer(const vector_type& extent, T fill = T());
        Buffer(const Buffer&) = default;
        Buffer(Buffer&&) noexcept = default;
        ~Buffer() = default;

        Buffer& operator=(const Buffer&) = default;
        Buffer& operator=(Buffer&&) noexcept = default;

        const vector_type& extent() const;
        bool contains(const point_type& position) const;

        T at(const point_type& position) const;
        void set(const point_type& position, T value);

        void fill(T value);

        // Highest row first
        std::string to_string() const requires std::same_as<T, char>;

    private:

        size_t offset(const point_type& position) const;

        vector_type m_extent;
        std::vector<T> m_values;
    };

    //--------------------------------------------------------------------------
    // A run of cells within one row
    struct Span
    {
        point_type position;
        coord_type count = 0;

        friend bool operator==(const Span&, const Span&) = default;
    };

    using spans_type = std::vector<Span>;

    // The runs in which b differs from a, lowest row first. Empty when they are the same, and the whole of b when they
    // differ in size
    template<typename T>
    spans_type difference(const Buffer<T>& a, const Buffer<T>& b);

} // namespace quetzal::stratum_view

//------------------------------------------------------------------------------
template<typename T>
quetzal::stratum_view::Buffer<T>::Buffer(const vector_type& extent, T fill) :
    m_extent(extent),
    m_values(static_cast<size_t>(extent.x() * extent.y()), fill)
{
}

//------------------------------------------------------------------------------
template<typename T>
const quetzal::stratum_view::vector_type& quetzal::stratum_view::Buffer<T>::extent() const
{
    return m_extent;
}

//------------------------------------------------------------------------------
template<typename T>
bool quetzal::stratum_view::Buffer<T>::contains(const point_type& position) const
{
    return position.i() >= 0 && position.i() < m_extent.x()
        && position.j() >= 0 && position.j() < m_extent.y();
}

//------------------------------------------------------------------------------
template<typename T>
T quetzal::stratum_view::Buffer<T>::at(const point_type& position) const
{
    return m_values[offset(position)];
}

//------------------------------------------------------------------------------
template<typename T>
void quetzal::stratum_view::Buffer<T>::set(const point_type& position, T value)
{
    m_values[offset(position)] = value;
    return;
}

//------------------------------------------------------------------------------
template<typename T>
void quetzal::stratum_view::Buffer<T>::fill(T value)
{
    std::fill(m_values.begin(), m_values.end(), value);
    return;
}

//------------------------------------------------------------------------------
template<typename T>
std::string quetzal::stratum_view::Buffer<T>::to_string() const requires std::same_as<T, char>
{
    std::string s;
    s.reserve(static_cast<size_t>(m_extent.y() * (m_extent.x() + 1)));

    for (coord_type j = m_extent.y() - 1; j >= 0; --j)
    {
        for (coord_type i = 0; i < m_extent.x(); ++i)
        {
            s += m_values[offset({i, j})];
        }

        s += '\n';
    }

    return s;
}

//------------------------------------------------------------------------------
template<typename T>
size_t quetzal::stratum_view::Buffer<T>::offset(const point_type& position) const
{
    assert(contains(position));
    return static_cast<size_t>(position.j() * m_extent.x() + position.i());
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::stratum_view::spans_type quetzal::stratum_view::difference(const Buffer<T>& a, const Buffer<T>& b)
{
    spans_type spans;

    const vector_type& extent = b.extent();

    if (a.extent() != extent)
    {
        for (coord_type j = 0; j < extent.y(); ++j)
        {
            spans.push_back(Span {{0, j}, extent.x()});
        }

        return spans;
    }

    for (coord_type j = 0; j < extent.y(); ++j)
    {
        coord_type i = 0;

        while (i < extent.x())
        {
            if (a.at({i, j}) == b.at({i, j}))
            {
                ++i;
                continue;
            }

            coord_type i_first = i;
            while (i < extent.x() && a.at({i, j}) != b.at({i, j}))
            {
                ++i;
            }

            spans.push_back(Span {{i_first, j}, i - i_first});
        }
    }

    return spans;
}

#endif // QUETZAL_STRATUM_VIEW_BUFFER_HPP
