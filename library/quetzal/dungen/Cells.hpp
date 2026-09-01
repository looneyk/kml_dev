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

        explicit Cells(dimensions_type size);
        Cells(dimensions_type size, Cell cell, int);
        Cells(dimensions_type size, std::initializer_list<Cell> cells);
        Cells(const Cells&) = default;
        ~Cells() = default;

        Cells& operator=(const Cells&) = default;

        dimensions_type size() const;

        const Cell& cell(position_type position) const;
        Cell& cell(position_type position);

        void print(std::ostream& os) const;
        void print(std::ostream& os, position_type position, dimensions_type size) const;

    private:

        dimensions_type m_size;
        cells_type m_cells;
    };

    std::ostream& operator<<(std::ostream& os, const Cells& cells);

} // namespace quetzal::dungen

#endif // QUETZAL_DUNGEN_CELLS_HPP
