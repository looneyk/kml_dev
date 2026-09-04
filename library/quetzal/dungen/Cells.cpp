//------------------------------------------------------------------------------
// dungen
// Cells.cpp
//------------------------------------------------------------------------------

#include "Cells.hpp"
#include <iostream>
#include <cassert>

using namespace std;

//------------------------------------------------------------------------------
quetzal::dungen::Cells::Cells(dimensions_type size) :
    m_size(size),
    m_cells(size.x() * size.y())
{
}

//------------------------------------------------------------------------------
quetzal::dungen::Cells::Cells(dimensions_type size, Cell cell, int) :
    m_size(size),
    m_cells(size.x() * size.y(), cell)
{
}

//------------------------------------------------------------------------------
quetzal::dungen::Cells::Cells(dimensions_type size, initializer_list<Cell> cells) :
    m_size(size),
    m_cells(cells)
{
    assert(cells.size() == size.x() * size.y());
}

//------------------------------------------------------------------------------
quetzal::dungen::dimensions_type quetzal::dungen::Cells::size() const
{
    return m_size;
}

//------------------------------------------------------------------------------
const quetzal::dungen::Cell& quetzal::dungen::Cells::cell(position_type position) const
{
    assert(position.x() < m_size.x());
    assert(position.y() < m_size.y());
    return m_cells[m_size.x() * position.y() + position.x()];
}

//------------------------------------------------------------------------------
quetzal::dungen::Cell& quetzal::dungen::Cells::cell(position_type position)
{
    assert(position.x() < m_size.x());
    assert(position.y() < m_size.y());
    return m_cells[m_size.x() * position.y() + position.x()];
}

//------------------------------------------------------------------------------
void quetzal::dungen::Cells::print(ostream& os) const
{
    print(os, {0, 0}, size());
    return;
}

//------------------------------------------------------------------------------
void quetzal::dungen::Cells::print(std::ostream& os, position_type position, dimensions_type size) const
{
    auto [nx, ny] = size;
    os << "(" << position.x() << ", " << position.y() << ")\t(" << nx << ", " << ny << ")" << endl;

    for (coord_type y = ny - 1; y >= 0; --y)
    {
        for (coord_type x = 0; x < nx; ++x)
        {
            cell({x, y}).render(os);
        }

        os << endl;
    }

    return;
}

//------------------------------------------------------------------------------
ostream& quetzal::dungen::operator<<(ostream& os, const Cells& cells)
{
    cells.print(os);
    return os;
}
