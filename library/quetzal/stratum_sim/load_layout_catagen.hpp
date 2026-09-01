#if !defined(QUETZAL_STRATUM_SIM_LOAD_LAYOUT_CATAGEN_HPP)
#define QUETZAL_STRATUM_SIM_LOAD_LAYOUT_CATAGEN_HPP
//------------------------------------------------------------------------------
// stratum_sim
// load_layout_catagen.hpp
//------------------------------------------------------------------------------

#include "load_layout.hpp"
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace quetzal::stratum_sim::detail
{

    // catagen's wall format puts the walls between the cells, so the grid is two characters
    // across per cell plus one: cell (i, j) is at (2i + 1, 2j + 1), the wall west of it at
    // (2i, 2j + 1), the wall south of it at (2i + 1, 2j), and their corners at the even
    // positions in both, which are read as decoration. A cell is ' ' open, '#' rock, or '>'
    // and '<' stairs, which are floor until something reads them as more; a wall is ' '
    // absent, '_' or '|' solid, '#' buried in rock, or '/' a door
    // The first line given is the highest row, the grid origin being lower left

    // The extent the file covers: what its header says, or what its own geometry says when
    // the header does not say it
    // Throws std::runtime_error when the header's extent is unreadable, or when there is
    // none and the grid is not a wall grid
    std::pair<size_t, size_t> wall_extent(const Header& header, const std::vector<std::string>& lines,
        const std::filesystem::path& filepath);

    // Cells are joined into a component only across a wall that is absent, so a door
    // separates two of them, and only a door is a passage a mover may cross
    // Throws std::runtime_error when the grid does not match the extent or holds a symbol
    // this does not read
    Map read_walls(const std::vector<std::string>& lines, size_t w, size_t h, const std::filesystem::path& filepath);

} // namespace quetzal::stratum_sim::detail

#endif // QUETZAL_STRATUM_SIM_LOAD_LAYOUT_CATAGEN_HPP
