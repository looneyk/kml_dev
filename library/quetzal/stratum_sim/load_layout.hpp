#if !defined(QUETZAL_STRATUM_SIM_LOAD_LAYOUT_HPP)
#define QUETZAL_STRATUM_SIM_LOAD_LAYOUT_HPP
//------------------------------------------------------------------------------
// stratum_sim
// load_layout.hpp
//------------------------------------------------------------------------------

#include "DiscreteLayout.hpp"
#include <filesystem>
#include <functional>
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
    // '0' null are both solid, ' ' room and '.' corridor are open floor, '|' is a doorway and
    // '>' a stairway. Nothing separates two open cells, so a doorway or a stairway is a block: a
    // cell of wall a mover may stand in, belonging to the area around it, walled on the sides
    // it presents to solid cells by that area's own outline and ringed with four walls of its own
    // the door hangs on. Two areas never share an edge, a block always lying between
    // Room floor and corridor floor are laid out alike and differ only in what the level
    // calls them, which is the one property either format encodes about rooms
    //
    // catagen's wall format puts the walls between the cells, so the grid is two characters
    // across per cell plus one: cell (i, j) is at (2i + 1, 2j + 1), the wall west of it at
    // (2i, 2j + 1), the wall south of it at (2i + 1, 2j), and their corners at the even
    // positions in both, which are read as decoration. A cell is ' ' open, '#' solid, or
    // '>' and '<' stairs, which are floor until something reads them as more; a wall is
    // ' ' absent, '-' or '|' solid, '#' where the position is solid instead, or '/' a door. Its
    // extent comes from the header, and where the header does not name one it is derived from
    // the grid: the last column is a boundary wall or one of its corners and so is never blank,
    // which means no row can have been trimmed and the widest is the width. Given one, the extent
    // is also what a truncated file is caught by
    //
    // Cells with nothing between them become one area, its boundary traced into one outline
    // per closed run of it. Two areas meeting are partnered wherever they touch, and
    // passable only where the format marks a crossing
    // A Room is built per contiguous run of the cells the format called room floor, and a
    // format that defines none builds none
    //
    //--------------------------------------------------------------------------
    // Told of each pair of walls the file called a door, as the walls are built. The loader
    // builds geometry and has no world to build entities in, so it reports rather than places
    // -- which is what lets a caller with a world attach a door without the loader having any
    // notion of what a door is
    // One wall per side it presents: two for a door in a wall and for a doorway between two
    // areas, one for a doorway into a dead end
    using door_type = std::function<void(const std::vector<id_type>& wall_ids)>;

    //--------------------------------------------------------------------------
    // Which of the two a file was written in, told by the header naming its generator and a
    // file with no header being the block format. Both load into the same structure, so this
    // is for a caller that has something else to decide by it -- what a view of it shows
    // without losing anything, being the one so far
    enum class Format
    {
        Block,
        Wall
    };

    // Both formats are grids of cells, so both build a DiscreteLayout and this declares it
    // rather than returning the interface. A format describing something else is a
    // reader of its own rather than another return from this one
    //
    // Throws std::runtime_error when the file cannot be opened, has no rows, names a
    // generator or version this does not read, or does not match its own header. Nothing is
    // built until every one of those has passed, so a throw leaves no half-built layout
    // format, where given, is told which of the two was read, and is left alone by a throw
    std::unique_ptr<DiscreteLayout> load_layout(const std::filesystem::path& filepath,
        value_type height_ceiling = value_type(1), const door_type& door = {}, Format* format = nullptr);

    //--------------------------------------------------------------------------
    // What a format reader produces and what reading one needs. Shared by the readers in
    // load_layout_dungen.hpp and load_layout_catagen.hpp, and by nothing outside them
    namespace detail
    {

        constexpr size_t npos = static_cast<size_t>(-1);

        //----------------------------------------------------------------------
        struct Edge
        {
            discrete_point_type grid_position_from;
            discrete_point_type grid_position_to;
        };

        //----------------------------------------------------------------------
        // Somewhere two components meet. Whether a mover may cross is the format's to decide: a
        // block layout separates by spending a cell, a wall layout by what is on the edge
        struct Passage
        {
            discrete_point_type grid_position;
            discrete_point_type grid_position_other;
            bool passable = false;
        };

        //----------------------------------------------------------------------
        // A cell of wall a mover may stand in: the block format's doorway and stairway. It is
        // part of the area around it, so that area's outline walls the sides it presents to
        // solid cells, and a ring of its own supplies the faces it presents to floor. Those are wound
        // about the floor either side, so a door hangs on them and a mover inside meets only
        // backs -- which are not walls to a query and so do not stop it leaving
        struct Block
        {
            discrete_point_type grid_position;
            std::vector<discrete_point_type> neighbor_grid_positions; // The open cells its faces adjoin
        };

        //----------------------------------------------------------------------
        // What both formats reduce to before a layout is built: which cells are open, which
        // of them belong together, and where two of those groups meet
        struct Map
        {
            discrete_vector_type extent;

            std::vector<bool> cell_open; // By extent, true where a mover may stand

            std::vector<size_t> cell_components; // By extent, npos where the cell is not in one
            std::vector<std::vector<discrete_point_type>> component_cells; // name? ...
            std::vector<Passage> passages;
            std::vector<Block> blocks;

            // By extent, true where the format said the cell is room floor rather than
            // corridor, doorway or anything else open. Empty when the format defines no
            // rooms at all, which loads a zone with none
            std::vector<bool> cell_room;

            size_t slot(const discrete_point_type& grid_position) const;
            size_t cell_count() const;
        };

        //----------------------------------------------------------------------
        inline size_t Map::slot(const discrete_point_type& grid_position) const
        {
            return static_cast<size_t>(grid_position.j() * extent.x() + grid_position.i());
        }

        //----------------------------------------------------------------------
        inline size_t Map::cell_count() const
        {
            return static_cast<size_t>(extent.x() * extent.y());
        }

        //----------------------------------------------------------------------
        // The row a generator writes ahead of the grid, naming itself, the version of the
        // format it wrote, and the parameters of the run as name=value tokens
        // The version is a string rather than a number: it is a label a reader matches, and a
        // pre-release one has a dot
        struct Header
        {
            std::string generator;
            std::string version;
            std::map<std::string, std::string> values;
        };

        //----------------------------------------------------------------------
        // Groups open cells, a caller's predicate deciding whether two adjacent ones belong
        // together and whether a cell is open at all
        template<typename Open, typename Joined>
        void find_components(Map& map, Open open, Joined joined)
        {
            map.cell_components.assign(map.cell_count(), npos);

            for (coord_type j = 0; j < map.extent.y(); ++j)
            {
                for (coord_type i = 0; i < map.extent.x(); ++i)
                {
                    discrete_point_type grid_position_start {i, j};
                    size_t slot = map.slot(grid_position_start);

                    if (!open(i, j) || map.cell_components[slot] != npos)
                    {
                        continue;
                    }

                    size_t component = map.component_cells.size();
                    map.component_cells.push_back({});

                    std::vector<discrete_point_type> pending {grid_position_start};
                    map.cell_components[slot] = component;

                    while (!pending.empty())
                    {
                        auto grid_position = pending.back();
                        pending.pop_back();
                        map.component_cells[component].push_back(grid_position);

                        for (const auto& direction : cardinal_directions)
                        {
                            discrete_point_type neighbor = grid_position + direction;

                            if (neighbor.i() < 0 || neighbor.j() < 0 || neighbor.i() >= map.extent.x() || neighbor.j() >= map.extent.y())
                            {
                                continue;
                            }

                            if (!open(neighbor.i(), neighbor.j()) || !joined(grid_position, neighbor))
                            {
                                continue;
                            }

                            size_t neighbor_slot = map.slot(neighbor);
                            if (map.cell_components[neighbor_slot] == npos)
                            {
                                map.cell_components[neighbor_slot] = component;
                                pending.push_back(neighbor);
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
