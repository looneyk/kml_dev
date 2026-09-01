//------------------------------------------------------------------------------
// dungen
// Pattern.cpp
//------------------------------------------------------------------------------

#include "Pattern.hpp"
#include <array>
#include <functional>

#include <iostream>
#include <cassert>

using namespace std;

namespace
{

    using quetzal::dungen::position_type;
    using quetzal::dungen::dimensions_type;

    const array<function<position_type(position_type, dimensions_type)>, 4> rotator =
    {
        [](position_type position, dimensions_type size) -> position_type { size; return {position.x(), position.y()}; },
        [](position_type position, dimensions_type size) -> position_type { return {size.y() - 1 - position.y(), position.x()}; },
        [](position_type position, dimensions_type size) -> position_type { return {size.x() - 1 - position.x(), size.y() - 1 - position.y()}; },
        [](position_type position, dimensions_type size) -> position_type { return {position.y(), size.x() - 1 - position.x()}; },
    };

    //--------------------------------------------------------------------------
    bool contains(position_type position, dimensions_type dimensions)
    {
        return position.x() >= 0 && position.x() <= dimensions.x() && position.y() >= 0 && position.y() <= dimensions.y();
    }

    //--------------------------------------------------------------------------
    quetzal::dungen::vector_type rotate(const quetzal::dungen::vector_type& v, size_t rotation)
    {
        switch (rotation % 4)
        {
            case 0:
                return v;

            case 1:
                return {v.y(), -v.x()};

            case 2:
                return -v;

            case 3:
                return {-v.y(), v.x()};
        }

        return {};
    }

} // namespace

//------------------------------------------------------------------------------
ostream& quetzal::dungen::operator<<(ostream& os, const Pattern& pattern)
{
    os << pattern.m_cells;
    return os;
}

//------------------------------------------------------------------------------
bool quetzal::dungen::match(const Cells& cells, position_type position, const Pattern& pattern, size_t rotation)
{
    vector_type origin = rotator[rotation](pattern.m_origin, pattern.m_cells.size());
    if (!contains(position - origin, cells.size() - pattern.m_cells.size()))
    {
        return false;
    }

    auto [dx, dy] = pattern.m_cells.size();
    for (coord_type y = 0; y < dy; ++y)
    {
        for (coord_type x = 0; x < dx; ++x)
        {
            position_type cell_position = rotator[rotation]({x, y}, pattern.m_cells.size());
            Cell cell_pattern = pattern.m_cells.cell(cell_position);
            if (cell_pattern.type() != CellType::Null)
            {
                if (cells.cell(position - pattern.m_origin + position_type{x, y}).type() != cell_pattern.type())
                {
                    return false;
                }
            }
        }
    }

    return true;
}

//------------------------------------------------------------------------------
bool quetzal::dungen::match(const Cells& cells, position_type position, const Pattern& pattern)
{
    for (size_t rotation = 0; rotation < 4; ++rotation)
    {
        if (match(cells, position, pattern, rotation))
        {
            return true;
        }
    }

    return false;
}

//------------------------------------------------------------------------------
void quetzal::dungen::replace(Cells& cells, position_type position, const Pattern& pattern, size_t rotation)
{
    vector_type origin = rotator[rotation](pattern.m_origin, pattern.m_cells.size());
    if (!contains(position - origin, cells.size() - pattern.m_cells.size()))
    {
        return;
    }

    auto [dx, dy] = pattern.m_cells.size();
    for (coord_type y = 0; y < dy; ++y)
    {
        for (coord_type x = 0; x < dx; ++x)
        {
            position_type cell_position = rotator[rotation]({x, y}, pattern.m_cells.size());
            Cell cell_pattern = pattern.m_cells.cell(cell_position);
            if (cell_pattern.type() != CellType::Null)
            {
                cells.cell(position - pattern.m_origin + position_type{x, y}).set_type(cell_pattern.type());
            }
        }
    }

    return;
}

//------------------------------------------------------------------------------
tuple<bool, quetzal::dungen::vector_type> quetzal::dungen::match_replace(Cells& cells, position_type position, const Pattern& matcher, const Pattern& replacer)
{
    for (size_t rotation = 0; rotation < 4; ++rotation)
    {
        if (match(cells, position, matcher, rotation))
        {
            replace(cells, position, replacer, rotation);
            return {true, rotate(replacer.m_shift, rotation)};
        }
    }

    return {false, {0, 0}};
}
