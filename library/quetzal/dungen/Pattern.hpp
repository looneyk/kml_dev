#if !defined(QUETZAL_DUNGEN_PATTERN_HPP)
#define QUETZAL_DUNGEN_PATTERN_HPP
//------------------------------------------------------------------------------
// dungen
// Pattern.hpp
//------------------------------------------------------------------------------

#include "Cells.hpp"
#include "types.hpp"
#include <iosfwd>
#include <tuple>

namespace quetzal::dungen
{

    //--------------------------------------------------------------------------
    struct Pattern
    {
        Cells m_cells;
        vector_type m_origin;
        vector_type m_shift;
    };

    std::ostream& operator<<(std::ostream& os, const Pattern& pattern);

    bool match(const Cells& cells, position_type position, const Pattern& pattern, size_t rotation);
    bool match(const Cells& cells, position_type position, const Pattern& pattern);

    void replace(Cells& cells, position_type position, const Pattern& pattern, size_t rotation);
    std::tuple<bool, vector_type> match_replace(Cells& cells, position_type position, const Pattern& matcher, const Pattern& replacer);

} // namespace quetzal::dungen

#endif // QUETZAL_DUNGEN_PATTERN_HPP
