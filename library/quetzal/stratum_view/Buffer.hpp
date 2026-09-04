#if !defined(QUETZAL_STRATUM_VIEW_CONSOLE_HPP)
#define QUETZAL_STRATUM_VIEW_CONSOLE_HPP
//------------------------------------------------------------------------------
// stratum_view
// Console.hpp
//------------------------------------------------------------------------------

#include <concepts>
#include <string>
#include <vector>
#include <cassert>

namespace quetzal::stratum_view
{

    //--------------------------------------------------------------------------
    // A grid of cells to be drawn, not a device that draws them. The term is libtcod's and
    // curses', where a console is an offscreen buffer that is later blitted
    // Row 0 is the lowest, matching the simulation, so writing it out reverses them
    // Templated over what a cell holds because a view consumes what occupies a cell, not
    // characters: the terminal wants characters, a tile view wants what to look the tile up by
    template<typename T>
    class Buffer
    {
    public:

        using value_type = T;

        Buffer() = default;
        Buffer(size_t width, size_t height, T fill = T());
        Buffer(const Buffer&) = default;
        Buffer(Buffer&&) noexcept = default;
        ~Buffer() = default;

        Buffer& operator=(const Buffer&) = default;
        Buffer& operator=(Buffer&&) noexcept = default;

        size_t width() const;
        size_t height() const;

        T at(size_t i, size_t j) const;
        void set(size_t i, size_t j, T value);

        void fill(T value);

        // Highest row first, so that it reads as a map. Characters only, there being nothing
        // to write for a cell that does not hold one
        std::string to_string() const requires std::same_as<T, char>;

    private:

        size_t m_width = 0;
        size_t m_height = 0;
        std::vector<T> m_values;
    };

    using Console = Buffer<char>;

    //--------------------------------------------------------------------------
    // A run of cells within one row
    struct Span
    {
        size_t i = 0;
        size_t j = 0;
        size_t count = 0;

        friend bool operator==(const Span&, const Span&) = default;
    };

    using spans_type = std::vector<Span>;

    // The runs in which b differs from a, in row order, lowest row first. Empty when the
    // two are the same, and the whole of b when they differ in size, which a caller
    // repainting a device should take as its cue to clear first
    template<typename T>
    spans_type difference(const Buffer<T>& a, const Buffer<T>& b);

} // namespace quetzal::stratum_view

//------------------------------------------------------------------------------
template<typename T>
quetzal::stratum_view::Buffer<T>::Buffer(size_t width, size_t height, T fill) :
    m_width(width),
    m_height(height),
    m_values(width * height, fill)
{
}

//------------------------------------------------------------------------------
template<typename T>
size_t quetzal::stratum_view::Buffer<T>::width() const
{
    return m_width;
}

//------------------------------------------------------------------------------
template<typename T>
size_t quetzal::stratum_view::Buffer<T>::height() const
{
    return m_height;
}

//------------------------------------------------------------------------------
template<typename T>
T quetzal::stratum_view::Buffer<T>::at(size_t i, size_t j) const
{
    assert(i < m_width && j < m_height);
    return m_values[j * m_width + i];
}

//------------------------------------------------------------------------------
template<typename T>
void quetzal::stratum_view::Buffer<T>::set(size_t i, size_t j, T value)
{
    assert(i < m_width && j < m_height);
    m_values[j * m_width + i] = value;

    return;
}

//------------------------------------------------------------------------------
template<typename T>
void quetzal::stratum_view::Buffer<T>::fill(T value)
{
    m_values.assign(m_width * m_height, value);
    return;
}

//------------------------------------------------------------------------------
template<typename T>
std::string quetzal::stratum_view::Buffer<T>::to_string() const requires std::same_as<T, char>
{
    std::string s;
    s.reserve(m_height * (m_width + 1));

    for (size_t j = m_height; j-- > 0; )
    {
        s.append(&m_values[j * m_width], m_width);
        s += '\n';
    }

    return s;
}

//------------------------------------------------------------------------------
template<typename T>
quetzal::stratum_view::spans_type quetzal::stratum_view::difference(const Buffer<T>& a, const Buffer<T>& b)
{
    spans_type spans;

    if (a.width() != b.width() || a.height() != b.height())
    {
        for (size_t j = 0; j < b.height(); ++j)
        {
            spans.push_back(Span {0, j, b.width()});
        }

        return spans;
    }

    for (size_t j = 0; j < b.height(); ++j)
    {
        size_t i = 0;

        while (i < b.width())
        {
            if (a.at(i, j) == b.at(i, j))
            {
                ++i;
                continue;
            }

            size_t i_first = i;
            while (i < b.width() && a.at(i, j) != b.at(i, j))
            {
                ++i;
            }

            spans.push_back(Span {i_first, j, i - i_first});
        }
    }

    return spans;
}

#endif // QUETZAL_STRATUM_VIEW_CONSOLE_HPP
