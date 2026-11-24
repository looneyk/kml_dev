#if !defined(QUETZAL_DUNGEN_CELLS_HPP)
#define QUETZAL_DUNGEN_CELLS_HPP
//------------------------------------------------------------------------------
// dungen
// Cells.hpp
//------------------------------------------------------------------------------

#include "Cell.hpp"
#include "types.hpp"
#include <iosfwd>
#include <vector>

namespace quetzal::dungen
{

    //--------------------------------------------------------------------------
    class Cells
    {
    public:

        using cells_type = std::vector<Cell>;

        explicit Cells(discrete_vector2_type size);
        Cells(discrete_vector2_type size, Cell cell, int);
        Cells(discrete_vector2_type size, std::initializer_list<Cell> cells);
        Cells(const Cells&) = default;
        ~Cells() = default;

        Cells& operator=(const Cells&) = default;

        discrete_vector2_type size() const;
        bool contains(discrete_point2_type position) const;

        const Cell& cell(discrete_point2_type position) const;
        Cell& cell(discrete_point2_type position);

        void print(std::ostream& os) const;
        void print(std::ostream& os, const discrete_box2_type& rectangle) const;

    private:

        discrete_box2_type m_bounds;
        cells_type m_cells;
    };

    std::ostream& operator<<(std::ostream& os, const Cells& cells);

} // namespace quetzal::dungen

#endif // QUETZAL_DUNGEN_CELLS_HPP
