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

        explicit Cells(vector_type size);
        Cells(vector_type size, Cell cell, int);
        Cells(vector_type size, std::initializer_list<Cell> cells);
        Cells(const Cells&) = default;
        ~Cells() = default;

        Cells& operator=(const Cells&) = default;

        vector_type size() const;
        bool contains(point_type position) const;

        const Cell& cell(point_type position) const;
        Cell& cell(point_type position);

        void print(std::ostream& os) const;
        void print(std::ostream& os, const box_type& rectangle) const;

    private:

        box_type m_bounds;
        cells_type m_cells;
    };

    std::ostream& operator<<(std::ostream& os, const Cells& cells);

} // namespace quetzal::dungen

#endif // QUETZAL_DUNGEN_CELLS_HPP
