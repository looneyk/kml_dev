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
        discrete_vector2_type m_origin;
        discrete_vector2_type m_shift;
    };

    std::ostream& operator<<(std::ostream& os, const Pattern& pattern);

    bool match(const Cells& cells, discrete_point2_type position, const Pattern& pattern, size_t rotation);
    bool match(const Cells& cells, discrete_point2_type position, const Pattern& pattern);

    void replace(Cells& cells, discrete_point2_type position, const Pattern& pattern, size_t rotation);
    std::tuple<bool, discrete_vector2_type> match_replace(Cells& cells, discrete_point2_type position, const Pattern& matcher, const Pattern& replacer);

} // namespace quetzal::dungen

#endif // QUETZAL_DUNGEN_PATTERN_HPP
