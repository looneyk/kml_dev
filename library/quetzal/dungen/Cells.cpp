//------------------------------------------------------------------------------
// dungen
// Cells.cpp
//------------------------------------------------------------------------------

#include "Cells.hpp"
#include <iostream>
#include <cassert>

using namespace std;

//------------------------------------------------------------------------------
quetzal::dungen::Cells::Cells(vector_type size) :
    m_bounds(point_type{0, 0}, size),
    m_cells(size.x() * size.y())
{
}

//------------------------------------------------------------------------------
quetzal::dungen::Cells::Cells(vector_type size, Cell cell, int) :
    m_bounds(point_type{0, 0}, size),
    m_cells(size.x() * size.y(), cell)
{
}

//------------------------------------------------------------------------------
quetzal::dungen::Cells::Cells(vector_type size, initializer_list<Cell> cells) :
    m_bounds(point_type{0, 0}, size),
    m_cells(cells)
{
    assert(cells.size() == size.x() * size.y());
}

//------------------------------------------------------------------------------
quetzal::dungen::vector_type quetzal::dungen::Cells::size() const
{
    return m_bounds.extent();
}

//------------------------------------------------------------------------------
bool quetzal::dungen::Cells::contains(point_type position) const
{
    return m_bounds.contains(position);
}

//------------------------------------------------------------------------------
const quetzal::dungen::Cell& quetzal::dungen::Cells::cell(point_type position) const
{
    assert(contains(position));
    return m_cells[m_bounds.extent().x() * position.y() + position.x()];
}

//------------------------------------------------------------------------------
quetzal::dungen::Cell& quetzal::dungen::Cells::cell(point_type position)
{
    assert(contains(position));
    return m_cells[m_bounds.extent().x() * position.y() + position.x()];
}

//------------------------------------------------------------------------------
void quetzal::dungen::Cells::print(ostream& os) const
{
    print(os, {{0, 0}, size()});
    return;
}

//------------------------------------------------------------------------------
void quetzal::dungen::Cells::print(std::ostream& os, const box_type& rectangle) const
{
    auto [nx, ny] = rectangle.extent();
    os << "(" << rectangle.position().x() << ", " << rectangle.position().y() << ")\t(" << nx << ", " << ny << ")" << endl;

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
