#if !defined(QUETZAL_STRATUM_SIM_LOAD_LAYOUT_HPP)
#define QUETZAL_STRATUM_SIM_LOAD_LAYOUT_HPP
//------------------------------------------------------------------------------
// stratum_sim
// load_layout.hpp
//------------------------------------------------------------------------------

#include "DiscreteLayout.hpp"
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace quetzal::stratum_sim
{

    // Reads a level file in either of the two formats a generator writes, told apart by the
    // header row naming it. A file with no header is dungen's block format, which is what
    // every file written before headers existed is. The first line of the file is the
    // highest row in both, the grid origin being lower left
    //
    // dungen's block format is one character per cell in its own symbols: '#' solid and
    // '0' null are rock, ' ' room and '.' corridor are open floor, '|' is a doorway and
    // '>' a shaft. Nothing separates two open cells, so a doorway or a shaft is outlined on
    // its own even where nothing separates it, and every passage is one a mover may cross
    // Room floor and corridor floor are laid out alike and differ only in what the level
    // calls them, which is the one thing either format says about rooms
    //
    // catagen's wall format puts the walls between the cells, so the grid is two characters
    // across per cell plus one: cell (i, j) is at (2i + 1, 2j + 1), the wall west of it at
    // (2i, 2j + 1), the wall south of it at (2i + 1, 2j), and their corners at the even
    // positions in both, which are read as decoration. A cell is ' ' open, '#' rock, or
    // '>' and '<' stairs, which are floor until something reads them as more; a wall is
    // ' ' absent, '_' or '|' solid, '#' buried in rock, or '/' a door. Its extent comes from
    // the header, and where the header does not carry one it is derived from the grid: the
    // last column is a boundary wall or one of its corners and so is never blank, which
    // means no row can have been trimmed and the widest is the width. Given one, the extent
    // is also what a truncated file is caught by
    //
    // Cells with nothing between them become one area, its boundary traced into one outline
    // per closed run of it. Two areas meeting are partnered wherever they touch, and
    // passable only where the format says a mover may cross
    // A Room is built per contiguous run of the cells the format called room floor, and a
    // format that names none builds none
    //
    // Both formats are grids of cells, so both build a DiscreteLayout and this says so
    // rather than answering with the interface. A format describing something else is a
    // reader of its own rather than another return from this one
    //
    // Throws std::runtime_error when the file cannot be opened, holds no rows, names a
    // generator or version this does not read, or does not match its own header. Nothing is
    // built until every one of those has passed, so a throw leaves no half-built layout
    std::unique_ptr<DiscreteLayout> load_layout(const std::filesystem::path& filepath, value_type height_ceiling = value_type(1));

    //--------------------------------------------------------------------------
    // What a format reader produces and what reading one needs. Shared by the readers in
    // load_layout_dungen.hpp and load_layout_catagen.hpp, and by nothing outside them
    namespace detail
    {

        constexpr size_t npos = static_cast<size_t>(-1);

        //----------------------------------------------------------------------
        // Lattice point, so that boundary edges chain by exact comparison
        struct Vertex
        {
            int i = 0;
            int j = 0;

            friend auto operator<=>(const Vertex&, const Vertex&) = default;
        };

        //----------------------------------------------------------------------
        struct Edge
        {
            Vertex from;
            Vertex to;
        };

        //----------------------------------------------------------------------
        // Somewhere two components meet. Whether a mover may cross is the format's to say: a
        // block layout separates by spending a cell, a wall layout by what stands on the edge
        struct Passage
        {
            Vertex cell;
            Vertex other;
            bool passable = false;
        };

        //----------------------------------------------------------------------
        // What both formats reduce to before a layout is built: which cells are open, which
        // of them belong together, and where two of those groups meet
        struct Map
        {
            size_t w = 0;
            size_t h = 0;
            std::vector<size_t> cell_components; // w by h, npos where the cell is not open
            std::vector<std::vector<Vertex>> component_cells;
            std::vector<Passage> passages;

            // w by h, true where the format said the cell is room floor rather than
            // corridor, doorway or anything else open. Empty when the format names no
            // rooms at all, which loads a zone with none
            std::vector<bool> cell_room;
        };

        //----------------------------------------------------------------------
        // The row a generator writes ahead of the grid, naming itself, the version of the
        // format it wrote, and the parameters of the run as name=value tokens
        struct Header
        {
            std::string generator;
            int version = 0;
            std::map<std::string, std::string> values;
        };

        // Two whole numbers with a separator between them, as a header writes a pair
        bool read_pair(const std::string& s, char separator, long& first, long& second);

        //----------------------------------------------------------------------
        // Groups open cells, a caller's predicate saying whether two adjacent ones belong
        // together and whether a cell is open at all
        template<typename Open, typename Joined>
        void find_components(Map& map, Open open, Joined joined)
        {
            map.cell_components.assign(map.w * map.h, npos);

            for (size_t j = 0; j < map.h; ++j)
            {
                for (size_t i = 0; i < map.w; ++i)
                {
                    size_t slot = j * map.w + i;
                    if (!open(static_cast<int>(i), static_cast<int>(j)) || map.cell_components[slot] != npos)
                    {
                        continue;
                    }

                    size_t component = map.component_cells.size();
                    map.component_cells.push_back({});

                    std::vector<Vertex> pending {Vertex {static_cast<int>(i), static_cast<int>(j)}};
                    map.cell_components[slot] = component;

                    while (!pending.empty())
                    {
                        Vertex v = pending.back();
                        pending.pop_back();
                        map.component_cells[component].push_back(v);

                        const Vertex neighbors[4] {{v.i + 1, v.j}, {v.i - 1, v.j}, {v.i, v.j + 1}, {v.i, v.j - 1}};
                        for (const Vertex& n : neighbors)
                        {
                            if (n.i < 0 || n.j < 0 || n.i >= static_cast<int>(map.w) || n.j >= static_cast<int>(map.h))
                            {
                                continue;
                            }

                            if (!open(n.i, n.j) || !joined(v, n))
                            {
                                continue;
                            }

                            size_t n_slot = static_cast<size_t>(n.j) * map.w + n.i;
                            if (map.cell_components[n_slot] == npos)
                            {
                                map.cell_components[n_slot] = component;
                                pending.push_back(n);
                            }
                        }
                    }
                }
            }

            return;
        }

    } // namespace detail

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_LOAD_LAYOUT_HPP
