//------------------------------------------------------------------------------
// stratum_view
// Console.cpp
//------------------------------------------------------------------------------

#include "Console.hpp"
#include <cassert>

//------------------------------------------------------------------------------
quetzal::stratum_view::Console::Console(size_t width, size_t height, char fill) :
    m_width(width),
    m_height(height),
    m_characters(width * height, fill)
{
}

//------------------------------------------------------------------------------
size_t quetzal::stratum_view::Console::width() const
{
    return m_width;
}

//------------------------------------------------------------------------------
size_t quetzal::stratum_view::Console::height() const
{
    return m_height;
}

//------------------------------------------------------------------------------
char quetzal::stratum_view::Console::at(size_t i, size_t j) const
{
    assert(i < m_width && j < m_height);
    return m_characters[j * m_width + i];
}

//------------------------------------------------------------------------------
void quetzal::stratum_view::Console::set(size_t i, size_t j, char c)
{
    assert(i < m_width && j < m_height);
    m_characters[j * m_width + i] = c;

    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_view::Console::fill(char c)
{
    m_characters.assign(m_width * m_height, c);
    return;
}

//------------------------------------------------------------------------------
std::string quetzal::stratum_view::Console::to_string() const
{
    std::string s;
    s.reserve(m_height * (m_width + 1));

    for (size_t j = m_height; j-- > 0; )
    {
        s.append(&m_characters[j * m_width], m_width);
        s += '\n';
    }

    return s;
}

//------------------------------------------------------------------------------
quetzal::stratum_view::spans_type quetzal::stratum_view::difference(const Console& a, const Console& b)
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
