//------------------------------------------------------------------------------
// stratum_format
// read_zone.cpp
//------------------------------------------------------------------------------

#include "read_zone.hpp"
#include "grid.hpp"
#include "checked_values.hpp"
#include "references.hpp"
#include "quetzal/geometry/DiscreteBox.hpp"
#include "quetzal/log/Log.hpp"
#include <algorithm>
#include <format>
#include <fstream>
#include <functional>
#include <iterator>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>
#include <cassert>
#include <cctype>
#include <cstdint>
#include <cstdlib>

namespace
{

    using namespace quetzal::stratum_format;
    using namespace quetzal::stratum_format::detail;
    using quetzal::id_type;

    using discrete_box_type = quetzal::geometry::DiscreteBox<coord_type, 2>;

    //--------------------------------------------------------------------------
    // A barrier with a normal is one face of the cell; without one it is the whole cell
    Barrier read_barrier(const raw_values_type& v, const std::string& path)
    {
        object(v, path);

        Barrier barrier {read_cell_position(required_member(v, path, "cell_position"), member_path(path, "cell_position")), {}};

        if (const raw_values_type* p = member(v, "normal"); p != nullptr)
        {
            barrier.normal = read_direction(*p, member_path(path, "normal"));
        }

        return barrier;
    }

    //--------------------------------------------------------------------------
    // The glyphs a position in the notation accepts. A corner accepts only a space
    std::string_view glyphs(Notation notation, bool cell_x, bool cell_y)
    {
        if (notation == Notation::Block)
        {
            return ".#+><";
        }

        if (cell_x && cell_y)
        {
            return ".>< ";
        }

        if (cell_x) // Horizontal, between the cell above and the cell below
        {
            return "-+ ";
        }

        if (cell_y) // Vertical, between the cell left and the cell right
        {
            return "|+ ";
        }

        return " ";
    }

    //--------------------------------------------------------------------------
    // A count repeats the glyph that follows it, and a glyph with no count is one of it
    std::string expand_runs(const std::string& row, size_t width, const std::string& path)
    {
        std::string expanded;

        for (size_t i = 0; i < row.size(); )
        {
            size_t n = 0;
            bool counted = false;

            while (i < row.size() && std::isdigit(static_cast<unsigned char>(row[i])) != 0)
            {
                n = n * 10 + static_cast<size_t>(row[i] - '0');
                if (n > width)
                {
                    fail(path, std::format("a run count past the row width of {}", width));
                }

                ++i;
                counted = true;
            }

            if (counted && n == 0)
            {
                fail(path, "a run count of zero");
            }

            if (i == row.size())
            {
                fail(path, "a run count with no glyph after it");
            }

            size_t run = counted ? n : 1;
            if (expanded.size() + run > width)
            {
                fail(path, std::format("a row of {} is expected, and this one expands past it", width));
            }

            expanded.append(run, row[i]);
            ++i;
        }

        return expanded;
    }

    //--------------------------------------------------------------------------
    void check_on_grid(const Cells& cells, const discrete_point_type& cell_position, const std::string& path)
    {
        if (!on_grid(cells, cell_position))
        {
            fail(path, "off the grid");
        }

        return;
    }

    //--------------------------------------------------------------------------
    // In the wall notation, a wall separates every floor cell from each blank cell, from each stair cell and from the edge of
    // the grid
    void check_floor_enclosed(const Cells& cells, const std::string& path)
    {
        if (cells.notation != Notation::Wall)
        {
            return;
        }

        for (coord_type j = 0; j < cells.extent.y(); ++j)
        {
            for (coord_type i = 0; i < cells.extent.x(); ++i)
            {
                discrete_point_type cell_position {i, j};
                if (!floor_cell(cells, cell_position))
                {
                    continue;
                }

                for (const auto& direction : quetzal::math::cardinal_directions<coord_type, 2>)
                {
                    discrete_point_type neighbor = cell_position + direction;
                    bool neighbor_on_grid = on_grid(cells, neighbor);

                    if ((neighbor_on_grid && floor_cell(cells, neighbor)) || has_wall(cells, cell_position, direction))
                    {
                        continue;
                    }

                    size_t x = static_cast<size_t>(2 * i + 1 + direction.x());
                    size_t y = static_cast<size_t>(2 * j + 1 + direction.y());
                    std::string beyond = !neighbor_on_grid ? "the edge of the grid" : stair_cell(cells, neighbor) ? "a stair cell" : "a blank cell";
                    fail(element_path(path, cells.rows.size() - 1 - y), std::format("no wall at {} between a floor cell and {}", x, beyond));
                }
            }
        }

        return;
    }

    //--------------------------------------------------------------------------
    // Checks the row count and width against the extent and notation, and each glyph against its position. The first row
    // is the highest
    void read_rows(const raw_values_type& v, const std::string& path, Cells& cells)
    {
        const raw_values_type& a = array(v, path);

        bool wall = cells.notation == Notation::Wall;
        size_t extent_x = static_cast<size_t>(cells.extent.x());
        size_t extent_y = static_cast<size_t>(cells.extent.y());
        size_t width = wall ? 2 * extent_x + 1 : extent_x;
        size_t count = wall ? 2 * extent_y + 1 : extent_y;

        if (a.size() != count)
        {
            fail(path, std::format("{} rows were expected, and there are {}", count, a.size()));
        }

        cells.rows.reserve(count);

        for (size_t i = 0; i < a.size(); ++i)
        {
            std::string row_path = element_path(path, i);
            std::string row = string_value(a[i], row_path);

            if (cells.encoding == Encoding::Runs)
            {
                row = expand_runs(row, width, row_path);
            }

            if (row.size() != width)
            {
                fail(row_path, std::format("a row of {} is expected, and this one is {}", width, row.size()));
            }

            size_t y = count - 1 - i;

            for (size_t x = 0; x < row.size(); ++x)
            {
                std::string_view permitted = glyphs(cells.notation, x % 2 == 1, y % 2 == 1);
                if (permitted.find(row[x]) == std::string_view::npos)
                {
                    fail(row_path, std::format("'{}' at {} is not a glyph that position accepts", row[x], x));
                }
            }

            cells.rows.push_back(std::move(row));
        }

        return;
    }

    //--------------------------------------------------------------------------
    Cells read_cells(const raw_values_type& v, const std::string& path)
    {
        object(v, path);

        Cells cells;

        std::string notation_path = member_path(path, "notation");
        std::optional<Notation> notation = to_notation(string_value(required_member(v, path, "notation"), notation_path));
        if (!notation.has_value())
        {
            fail(notation_path, "not a notation the format defines");
        }

        cells.notation = *notation;

        if (const raw_values_type* p = member(v, "encoding"); p != nullptr)
        {
            std::string encoding_path = member_path(path, "encoding");
            std::optional<Encoding> encoding = to_encoding(string_value(*p, encoding_path));
            if (!encoding.has_value())
            {
                fail(encoding_path, "not an encoding the format defines");
            }

            cells.encoding = *encoding;
        }

        std::string extent_path = member_path(path, "extent");
        const raw_values_type& extent = fixed_array(required_member(v, path, "extent"), extent_path, 2);
        cells.extent = {coord_value(extent[0], element_path(extent_path, 0)),
            coord_value(extent[1], element_path(extent_path, 1))};

        if (cells.extent.x() <= 0 || cells.extent.y() <= 0)
        {
            fail(extent_path, "an extent of no cells");
        }

        if (const raw_values_type* p = member(v, "origin"); p != nullptr)
        {
            cells.origin = read_position(*p, member_path(path, "origin"));
        }

        if (const raw_values_type* p = member(v, "scale"); p != nullptr)
        {
            cells.scale = number_value(*p, member_path(path, "scale"));
            if (cells.scale <= value_type(0))
            {
                fail(member_path(path, "scale"), "a scale must be positive");
            }
        }

        std::string rows_path = member_path(path, "rows");
        read_rows(required_member(v, path, "rows"), rows_path, cells);
        check_floor_enclosed(cells, rows_path);
        return cells;
    }

    //--------------------------------------------------------------------------
    // A wall is addressed from the cell it faces, which is a floor/door cell
    void check_wall_address(const Cells& cells, const WallAddress& address, const std::string& path)
    {
        check_on_grid(cells, address.cell_position, path);

        if (!floor_door_cell(cells, address.cell_position))
        {
            fail(path, "not a floor/door cell");
        }

        if (!has_wall(cells, address.cell_position, {-address.normal.x(), -address.normal.y()}))
        {
            fail(path, "no wall in the grid at that address");
        }

        return;
    }

    //--------------------------------------------------------------------------
    // Along cell edges, within the grid, and never crossing or touching itself. Every cell edge of it is a wall facing a
    // floor/door cell, or a virtual wall between two floor/door cells
    void check_perimeter(const Cells& cells, const Perimeter& perimeter, const std::string& path)
    {
        const std::vector<discrete_point_type>& junctions = perimeter.junctions;
        size_t n = junctions.size();

        for (size_t k = 0; k < n; ++k)
        {
            const discrete_point_type& a = junctions[k];
            const discrete_point_type& b = junctions[(k + 1) % n];

            if (a.i() < 0 || a.j() < 0 || a.i() > cells.extent.x() || a.j() > cells.extent.y())
            {
                fail(element_path(path, k), "off the grid");
            }

            if (a == b)
            {
                fail(element_path(path, k), "an edge of no length to the next junction");
            }

            if (a.i() != b.i() && a.j() != b.j())
            {
                fail(element_path(path, k), "an edge to the next junction that is neither horizontal nor vertical");
            }
        }

        auto sign = [](coord_type value) { return static_cast<coord_type>((value > 0) - (value < 0)); };

        for (size_t k = 0; k < n; ++k)
        {
            const discrete_point_type& a = junctions[k];
            const discrete_point_type& b = junctions[(k + 1) % n];

            for (size_t m = k + 1; m < n; ++m)
            {
                const discrete_point_type& c = junctions[m];
                const discrete_point_type& d = junctions[(m + 1) % n];

                if (m == k + 1 || (k == 0 && m == n - 1))
                {
                    bool doubles_back = sign(b.i() - a.i()) == -sign(d.i() - c.i()) && sign(b.j() - a.j()) == -sign(d.j() - c.j());
                    if (doubles_back)
                    {
                        fail(element_path(path, m == k + 1 ? m : 0), "the perimeter doubles back at this junction");
                    }

                    continue;
                }

                bool overlap_i = std::min(a.i(), b.i()) <= std::max(c.i(), d.i()) && std::min(c.i(), d.i()) <= std::max(a.i(), b.i());
                bool overlap_j = std::min(a.j(), b.j()) <= std::max(c.j(), d.j()) && std::min(c.j(), d.j()) <= std::max(a.j(), b.j());
                if (overlap_i && overlap_j)
                {
                    fail(element_path(path, m), std::format("the edge from this junction meets the edge from junction {}", k));
                }
            }
        }

        std::int64_t twice_area = 0;
        for (size_t k = 0; k < n; ++k)
        {
            const discrete_point_type& a = junctions[k];
            const discrete_point_type& b = junctions[(k + 1) % n];
            twice_area += static_cast<std::int64_t>(a.i()) * b.j() - static_cast<std::int64_t>(b.i()) * a.j();
        }

        // The inside is left of each edge on a counterclockwise perimeter and right of it on a clockwise one
        coord_type turn = twice_area > 0 ? 1 : -1;

        // A wall facing a floor/door cell on either side satisfies an edge, so that a pillar on a room's edge may lie inside the
        // perimeter. Which side is inside is not used yet, so a perimeter whose walls all face outward, enclosing no floor/door
        // cell, also passes, and a check of the enclosed area is needed to reject it ...
        for (size_t k = 0; k < n; ++k)
        {
            const discrete_point_type& a = junctions[k];
            const discrete_point_type& b = junctions[(k + 1) % n];

            discrete_vector_type step {sign(b.i() - a.i()), sign(b.j() - a.j())};
            discrete_vector_type inward {-turn * step.y(), turn * step.x()};
            discrete_vector_type outward {-inward.x(), -inward.y()};

            for (discrete_point_type junction = a; junction != b; junction += step)
            {
                discrete_point_type lower {std::min(junction.i(), junction.i() + step.x()), std::min(junction.j(), junction.j() + step.y())};
                discrete_point_type inside {lower.i() - (inward.x() < 0 ? 1 : 0), lower.j() - (inward.y() < 0 ? 1 : 0)};
                discrete_point_type outside = inside + outward;

                assert(on_grid(cells, inside));

                bool inside_open = floor_door_cell(cells, inside);
                bool outside_open = on_grid(cells, outside) && floor_door_cell(cells, outside);

                bool inside_wall = inside_open && has_wall(cells, inside, outward);
                bool outside_wall = outside_open && has_wall(cells, outside, inward);

                if (!(inside_open && outside_open) && !inside_wall && !outside_wall)
                {
                    fail(element_path(path, k), std::format("the edge from this junction between cell [{}, {}] inside and cell [{}, {}] outside is neither "
                        "a wall facing a floor/door cell nor between two of them", inside.i(), inside.j(), outside.i(), outside.j()));
                }
            }
        }

        return;
    }

    //--------------------------------------------------------------------------
    std::vector<Barrier> read_barriers(const raw_values_type& v, const std::string& path, const Cells& cells)
    {
        const raw_values_type& a = array(v, path);

        std::vector<Barrier> barriers;
        barriers.reserve(a.size());

        for (size_t i = 0; i < a.size(); ++i)
        {
            std::string barrier_path = element_path(path, i);
            Barrier barrier = read_barrier(a[i], barrier_path);
            check_on_grid(cells, barrier.cell_position, barrier_path);
            barriers.push_back(barrier);
        }

        return barriers;
    }

    //--------------------------------------------------------------------------
    Room read_room(const raw_values_type& v, const std::string& path, const Cells& cells)
    {
        object(v, path);

        const raw_values_type* perimeter = member(v, "perimeter");
        const raw_values_type* cell_position = member(v, "cell_position");

        if ((perimeter != nullptr) == (cell_position != nullptr))
        {
            fail(path, "one of perimeter and cell_position is given, and never both");
        }

        Room room {Perimeter {}, {}};

        if (perimeter != nullptr)
        {
            std::string perimeter_path = member_path(path, "perimeter");
            const raw_values_type& a = array(*perimeter, perimeter_path);

            if (a.size() < 4)
            {
                fail(perimeter_path, "a perimeter of fewer than four junctions");
            }

            Perimeter p;
            p.junctions.reserve(a.size());

            for (size_t i = 0; i < a.size(); ++i)
            {
                p.junctions.push_back(read_cell_position(a[i], element_path(perimeter_path, i)));
            }

            check_perimeter(cells, p, perimeter_path);

            room.identification = std::move(p);
        }
        else
        {
            std::string location_path = member_path(path, "cell_position");

            RoomLocator l;
            l.cell_position = read_cell_position(*cell_position, location_path);
            check_on_grid(cells, l.cell_position, location_path);

            if (!floor_cell(cells, l.cell_position))
            {
                fail(location_path, "not a floor cell");
            }

            if (const raw_values_type* p = member(v, "barriers"); p != nullptr)
            {
                l.barriers = read_barriers(*p, member_path(path, "barriers"), cells);
            }

            room.identification = std::move(l);
        }

        room.components = read_components(v, path);
        return room;
    }

    //--------------------------------------------------------------------------
    // Returns true if a barrier closes off the whole cell, leaving a fill nowhere to start
    bool location_barred(const std::vector<Barrier>& barriers, const discrete_point_type& cell_position)
    {
        return std::ranges::any_of(barriers, [&](const Barrier& barrier) { return !barrier.normal.has_value() && barrier.cell_position == cell_position; });
    }

    //--------------------------------------------------------------------------
    // Rejects two rooms containing one cell. Cells are compared only for rooms whose bounding boxes intersect
    void check_rooms(const Document& document)
    {
        for (size_t r = 0; r < document.rooms.size(); ++r)
        {
            const RoomLocator* locator = std::get_if<RoomLocator>(&document.rooms[r].identification);
            if (locator != nullptr && (location_barred(document.barriers, locator->cell_position) || location_barred(locator->barriers, locator->cell_position)))
            {
                fail(member_path(element_path("rooms", r), "cell_position"), "a location closed off by a barrier");
            }
        }

        std::vector<std::set<discrete_point_type>> covers = room_cells(document);

        std::vector<discrete_box_type> bounds;
        bounds.reserve(covers.size());

        for (const auto& cover : covers)
        {
            discrete_box_type box;
            for (const auto& cell_position : cover)
            {
                box = quetzal::geometry::op_union(box, discrete_box_type(cell_position, discrete_vector_type {1, 1}));
            }

            bounds.push_back(box);
        }

        for (size_t r = 0; r < covers.size(); ++r)
        {
            for (size_t q = 0; q < r; ++q)
            {
                if (quetzal::geometry::disjoint(bounds[r], bounds[q]))
                {
                    continue;
                }

                size_t smaller = covers[r].size() < covers[q].size() ? r : q;
                size_t larger = smaller == r ? q : r;

                for (const auto& cell_position : covers[smaller])
                {
                    if (bounds[larger].contains(cell_position) && covers[larger].contains(cell_position))
                    {
                        fail(element_path("rooms", r), std::format("covers the cell [{}, {}], which rooms[{}] also covers", cell_position.i(), cell_position.j(), q));
                    }
                }
            }
        }

        return;
    }

    //--------------------------------------------------------------------------
    Wall read_wall(const raw_values_type& v, const std::string& path, const Cells& cells)
    {
        object(v, path);

        Wall wall;
        std::string address_path = member_path(path, "wall");
        wall.address = read_wall_address(required_member(v, path, "wall"), address_path);
        check_wall_address(cells, wall.address, address_path);

        wall.components = read_components(v, path);
        return wall;
    }

    //--------------------------------------------------------------------------
    FlatProperties read_flat_properties(const raw_values_type& v, const std::string& path)
    {
        object(v, path);

        FlatProperties properties;

        if (const raw_values_type* p = member(v, "height"); p != nullptr)
        {
            properties.height = number_value(*p, member_path(path, "height"));
        }

        properties.components = read_components(v, path);
        return properties;
    }

    //--------------------------------------------------------------------------
    Flat read_flat(const raw_values_type& v, const std::string& path, const Cells& cells, size_t room_count)
    {
        object(v, path);

        Flat flat;

        std::string role_path = member_path(path, "role");
        std::optional<FlatRole> role = to_flat_role(string_value(required_member(v, path, "role"), role_path));
        if (!role.has_value())
        {
            fail(role_path, "not a role the format defines");
        }

        flat.role = *role;
        flat.properties = read_flat_properties(v, path);

        if (const raw_values_type* p = member(v, "rooms"); p != nullptr)
        {
            std::string rooms_path = member_path(path, "rooms");
            const raw_values_type& a = array(*p, rooms_path);

            for (size_t i = 0; i < a.size(); ++i)
            {
                std::string room_path = element_path(rooms_path, i);
                object(a[i], room_path);

                std::string id_path = member_path(room_path, "room");
                size_t room_id = count_value(required_member(a[i], room_path, "room"), id_path);
                if (room_id >= room_count)
                {
                    fail(id_path, "no room of that id");
                }

                if (!flat.rooms.emplace(room_id, read_flat_properties(a[i], room_path)).second)
                {
                    fail(id_path, "a second entry for one room");
                }
            }
        }

        if (const raw_values_type* p = member(v, "cells"); p != nullptr)
        {
            std::string cells_path = member_path(path, "cells");
            const raw_values_type& a = array(*p, cells_path);

            for (size_t i = 0; i < a.size(); ++i)
            {
                std::string cell_path = element_path(cells_path, i);
                object(a[i], cell_path);

                std::string position_path = member_path(cell_path, "cell_position");
                discrete_point_type cell_position = read_cell_position(required_member(a[i], cell_path, "cell_position"), position_path);
                check_on_grid(cells, cell_position, position_path);

                if (!flat.cells.emplace(cell_position, read_flat_properties(a[i], cell_path)).second)
                {
                    fail(position_path, "a second entry for one cell");
                }
            }
        }

        return flat;
    }

    //--------------------------------------------------------------------------
    // The address is a wall or a flat cell, and absent for an entity sited on its parent
    Site read_site(const raw_values_type& v, const std::string& path, const Cells& cells)
    {
        object(v, path);

        const raw_values_type* wall = member(v, "wall");
        const raw_values_type* flat = member(v, "flat");

        if (wall != nullptr && flat != nullptr)
        {
            fail(path, "wall and flat are never both given");
        }

        Site site;

        if (wall != nullptr)
        {
            std::string wall_path = member_path(path, "wall");
            WallAddress address = read_wall_address(*wall, wall_path);
            check_wall_address(cells, address, wall_path);
            site.address = address;
        }
        else if (flat != nullptr)
        {
            std::string role_path = member_path(path, "flat");
            std::optional<FlatRole> role = to_flat_role(string_value(*flat, role_path));
            if (!role.has_value())
            {
                fail(role_path, "not a role the format defines");
            }

            std::string cell_path = member_path(path, "cell_position");
            FlatCell flat_cell {*role, read_cell_position(required_member(v, path, "cell_position"), cell_path)};
            check_on_grid(cells, flat_cell.cell_position, cell_path);

            site.address = flat_cell;
        }
        else if (const raw_values_type* p = member(v, "cell_position"); p != nullptr)
        {
            std::string cell_path = member_path(path, "cell_position");
            ZoneCell zone_cell {read_cell_position(*p, cell_path)};
            check_on_grid(cells, zone_cell.cell_position, cell_path);

            if (!floor_door_cell(cells, zone_cell.cell_position))
            {
                fail(cell_path, "not a cell an actor can stand in");
            }

            site.address = zone_cell;
        }

        std::string relation_path = member_path(path, "relation");
        std::optional<Relation> relation = to_relation(string_value(required_member(v, path, "relation"), relation_path));
        if (!relation.has_value())
        {
            fail(relation_path, "not a relation the format defines");
        }

        site.relation = *relation;

        if (site.address.has_value() && std::holds_alternative<ZoneCell>(*site.address))
        {
            if (site.relation != Relation::In)
            {
                fail(relation_path, "an entity in a cell of the zone has the relation in");
            }

            if (member(v, "orientation") == nullptr)
            {
                fail(path, "an entity in a cell of the zone has no orientation");
            }
        }

        if (const raw_values_type* p = member(v, "position"); p != nullptr)
        {
            site.position = read_surface_position(*p, member_path(path, "position"));
        }

        if (const raw_values_type* p = member(v, "orientation"); p != nullptr)
        {
            site.orientation = read_direction(*p, member_path(path, "orientation"));
        }

        return site;
    }

    //--------------------------------------------------------------------------
    // Required, with at least one entry. The entity each entry refers to is checked once the entities are read
    std::vector<stored_entity_id_type> read_access_points(const raw_values_type& components)
    {
        const std::string path = "layout.components.access_points";

        const raw_values_type* p = components.is_object() ? member(components, "access_points") : nullptr;
        if (p == nullptr)
        {
            fail(path, "required and absent");
        }

        const raw_values_type& a = array(*p, path);
        if (a.empty())
        {
            fail(path, "no access point");
        }

        std::vector<stored_entity_id_type> access_points;

        for (size_t i = 0; i < a.size(); ++i)
        {
            std::optional<stored_entity_id_type> id = reference_id(a[i]);
            if (!id.has_value())
            {
                fail(element_path(path, i), "not a reference to an entity");
            }

            access_points.push_back(*id);
        }

        return access_points;
    }

    //--------------------------------------------------------------------------
    // Returns the level if the layout's components have one
    std::optional<level_type> read_level(const raw_values_type& components)
    {
        const raw_values_type* p = components.is_object() ? member(components, "level") : nullptr;
        return p != nullptr ? std::optional<level_type>(coord_value(*p, "layout.components.level")) : std::nullopt;
    }

    //--------------------------------------------------------------------------
    // Returns false if the layout's components do not have the flag
    bool read_unpopulated(const raw_values_type& components)
    {
        const raw_values_type* p = components.is_object() ? member(components, "unpopulated") : nullptr;
        return p != nullptr && boolean_value(*p, "layout.components.unpopulated");
    }

    //--------------------------------------------------------------------------
    // The type, order, and placement that an access point's relocator requires are checked with the access points
    Relocator read_relocator(const raw_values_type& v, const std::string& path, const Cells& cells)
    {
        object(v, path);

        Relocator relocator;

        std::string target_path = member_path(path, "target");
        const raw_values_type& target = required_member(v, path, "target");
        if (target.is_string())
        {
            std::optional<RelocatorTarget> parsed = parse_relocator_target(target.get<std::string>());
            if (!parsed.has_value())
            {
                fail(target_path, "not a target written [region/]zone[@position]");
            }

            relocator.target = *parsed;
        }
        else if (std::optional<stored_entity_id_type> id = reference_id(target); id.has_value())
        {
            relocator.target = *id;
        }
        else
        {
            fail(target_path, "neither a target nor a reference to an entity");
        }

        if (const raw_values_type* p = member(v, "type"); p != nullptr)
        {
            relocator.type = string_value(*p, member_path(path, "type"));
        }

        if (const raw_values_type* p = member(v, "order"); p != nullptr)
        {
            if (!p->is_number_unsigned())
            {
                fail(member_path(path, "order"), "a whole, non-negative number was expected");
            }

            relocator.order = p->get<std::uint64_t>();
        }

        if (const raw_values_type* p = member(v, "placement"); p != nullptr)
        {
            std::string placement_path = member_path(path, "placement");
            object(*p, placement_path);

            std::string cell_path = member_path(placement_path, "cell_position");
            discrete_point_type cell_position = read_cell_position(required_member(*p, placement_path, "cell_position"), cell_path);
            check_on_grid(cells, cell_position, cell_path);

            relocator.placement = discrete_directed_point_type(cell_position,
                read_direction(required_member(*p, placement_path, "orientation"), member_path(placement_path, "orientation")));
        }

        return relocator;
    }

    //--------------------------------------------------------------------------
    Entity read_entity(const raw_values_type& v, const std::string& path, const Cells& cells)
    {
        object(v, path);

        Entity entity;
        entity.id = id_value(required_member(v, path, "id"), member_path(path, "id"));
        entity.type = string_value(required_member(v, path, "type"), member_path(path, "type"));
        entity.components = read_components(v, path);

        if (entity.components.is_null())
        {
            return entity;
        }

        std::string components_path = member_path(path, "components");

        if (const raw_values_type* p = member(entity.components, "site"); p != nullptr)
        {
            entity.site = read_site(*p, member_path(components_path, "site"), cells);
        }

        if (const raw_values_type* p = member(entity.components, "parent"); p != nullptr)
        {
            std::optional<stored_entity_id_type> parent_id = reference_id(*p);
            if (!parent_id.has_value())
            {
                fail(member_path(components_path, "parent"), "not a reference to an entity");
            }

            entity.parent_id = *parent_id;
        }

        if (const raw_values_type* p = member(entity.components, "relocator"); p != nullptr)
        {
            entity.relocator = read_relocator(*p, member_path(components_path, "relocator"), cells);
        }

        entity.components.erase("site");
        entity.components.erase("parent");
        entity.components.erase("relocator");
        return entity;
    }

    using wall_key_type = std::array<coord_type, 4>;

    //--------------------------------------------------------------------------
    wall_key_type wall_key(const WallAddress& address)
    {
        return {address.cell_position.i(), address.cell_position.j(), address.normal.x(), address.normal.y()};
    }

    //--------------------------------------------------------------------------
    // Returns true if the entity is sited on the zone's geometry rather than on its parent
    bool sited_on_geometry(const Entity& entity)
    {
        return entity.site.has_value() && entity.site->address.has_value();
    }

    //--------------------------------------------------------------------------
    // Returns the wall an entity is sited on, or null if it is sited on a flat cell or on its parent
    const WallAddress* wall_address(const Entity& entity)
    {
        return sited_on_geometry(entity) ? std::get_if<WallAddress>(&*entity.site->address) : nullptr;
    }

    //--------------------------------------------------------------------------
    std::string wall_text(const WallAddress& address)
    {
        return std::format("[{}, {}] normal [{}, {}]", address.cell_position.i(), address.cell_position.j(),
            address.normal.x(), address.normal.y());
    }

    //--------------------------------------------------------------------------
    std::string walls_text(const std::vector<WallAddress>& addresses)
    {
        std::string text;
        for (size_t i = 0; i < addresses.size(); ++i)
        {
            text += std::format("{}{}", i == 0 ? "" : " and ", wall_text(addresses[i]));
        }

        return text;
    }

    //--------------------------------------------------------------------------
    // The entities by id, by the wall each is sited on, and by the parent each is a child of
    struct EntityIndex
    {
        std::map<stored_entity_id_type, size_t> slots_by_id;
        std::map<wall_key_type, std::vector<size_t>> slots_by_wall;
        std::map<stored_entity_id_type, std::vector<size_t>> slots_by_parent;
    };

    //--------------------------------------------------------------------------
    EntityIndex entity_index(const std::vector<Entity>& entities)
    {
        EntityIndex index;

        for (size_t slot = 0; slot < entities.size(); ++slot)
        {
            const Entity& entity = entities[slot];
            index.slots_by_id.emplace(entity.id, slot);

            if (const WallAddress* address = wall_address(entity); address != nullptr)
            {
                index.slots_by_wall[wall_key(*address)].push_back(slot);
            }

            if (entity.parent_id.has_value())
            {
                index.slots_by_parent[*entity.parent_id].push_back(slot);
            }
        }

        return index;
    }

    //--------------------------------------------------------------------------
    // Every entity is sited on the geometry, has a parent or is a parent, and every hierarchy of parents and
    // children is sited on the geometry somewhere in it. An entity whose parent is no entity in the file is the
    // root of its own hierarchy, as with a saved zone's reference into another zone
    void check_entities_placed(const std::vector<Entity>& entities, const EntityIndex& index)
    {
        for (size_t slot = 0; slot < entities.size(); ++slot)
        {
            const Entity& entity = entities[slot];

            if (entity.site.has_value() && !entity.site->address.has_value() && !entity.parent_id.has_value())
            {
                fail(element_path("entities", slot), "sited on its parent and has none");
            }

            if (!sited_on_geometry(entity) && !entity.parent_id.has_value() && !index.slots_by_parent.contains(entity.id))
            {
                fail(element_path("entities", slot), "not sited on the geometry, no entity's child and no entity's parent");
            }
        }

        for (size_t root = 0; root < entities.size(); ++root)
        {
            if (entities[root].parent_id.has_value() && index.slots_by_id.contains(*entities[root].parent_id))
            {
                continue;
            }

            bool sited = false;
            std::vector<size_t> pending {root};

            while (!pending.empty() && !sited)
            {
                size_t slot = pending.back();
                pending.pop_back();
                sited = sited_on_geometry(entities[slot]);

                if (auto i = index.slots_by_parent.find(entities[slot].id); i != index.slots_by_parent.end())
                {
                    pending.insert(pending.end(), i->second.begin(), i->second.end());
                }
            }

            if (!sited)
            {
                fail(element_path("entities", root), "no entity in this hierarchy of parents and children is sited on the geometry");
            }
        }

        return;
    }

    //--------------------------------------------------------------------------
    // Following parent references from an entity never leads back to it
    void check_entities_acyclic(const std::vector<Entity>& entities, const EntityIndex& index)
    {
        enum class Visit { None, Open, Done };
        std::vector<Visit> visits(entities.size(), Visit::None);
        std::vector<size_t> chain;

        for (size_t start = 0; start < entities.size(); ++start)
        {
            if (visits[start] != Visit::None)
            {
                continue;
            }

            chain.clear();

            for (size_t slot = start; visits[slot] == Visit::None; )
            {
                visits[slot] = Visit::Open;
                chain.push_back(slot);

                const std::optional<stored_entity_id_type>& parent_id = entities[slot].parent_id;
                auto next = parent_id.has_value() ? index.slots_by_id.find(*parent_id) : index.slots_by_id.end();
                if (next == index.slots_by_id.end())
                {
                    break;
                }

                if (visits[next->second] == Visit::Open)
                {
                    fail(element_path("entities", next->second), "its parents lead back to it");
                }

                slot = next->second;
            }

            for (const auto& slot : chain)
            {
                visits[slot] = Visit::Done;
            }
        }

        return;
    }

    //--------------------------------------------------------------------------
    // In a template every reference is an entity's id. A saved zone's references may point into another zone, so they are
    // not checked
    void check_references(const Document& document, const EntityIndex& index)
    {
        if (document.zone_id.has_value())
        {
            return;
        }

        auto check = [&](stored_entity_id_type id, const std::string& path)
        {
            if (!index.slots_by_id.contains(id))
            {
                fail(path, std::format("refers to {}, which is no entity's id", id));
            }
        };

        auto check_components = [&](const raw_values_type& components, const std::string& path)
        {
            for (stored_entity_id_type id : references(components))
            {
                check(id, member_path(path, "components"));
            }
        };

        check_components(document.components, "layout");

        for (size_t i = 0; i < document.rooms.size(); ++i)
        {
            check_components(document.rooms[i].components, element_path("rooms", i));
        }

        for (size_t i = 0; i < document.walls.size(); ++i)
        {
            check_components(document.walls[i].components, element_path("walls", i));
        }

        for (size_t i = 0; i < document.flats.size(); ++i)
        {
            std::string flat_path = element_path("flats", i);
            check_components(document.flats[i].properties.components, flat_path);

            for (const auto& [room_id, properties] : document.flats[i].rooms)
            {
                check_components(properties.components, std::format("{}.rooms[room {}]", flat_path, room_id));
            }

            for (const auto& [cell_position, properties] : document.flats[i].cells)
            {
                check_components(properties.components, std::format("{}.cells[cell [{}, {}]]", flat_path, cell_position.i(), cell_position.j()));
            }
        }

        for (size_t i = 0; i < document.entities.size(); ++i)
        {
            const Entity& entity = document.entities[i];
            std::string path = element_path("entities", i);

            check_components(entity.components, path);

            if (entity.parent_id.has_value())
            {
                check(*entity.parent_id, member_path(member_path(path, "components"), "parent"));
            }

            if (entity.relocator.has_value() && std::holds_alternative<stored_entity_id_type>(entity.relocator->target))
            {
                fail(member_path(member_path(member_path(path, "components"), "relocator"), "target"),
                    "bound, which a relocator is only in a saved zone");
            }
        }

        return;
    }

    //--------------------------------------------------------------------------
    // Each door glyph is one entity with a child sited on each wall of its doorway and on no other wall
    void check_door_glyphs(const Document& document, const EntityIndex& index)
    {
        auto child_on = [&](stored_entity_id_type parent, const WallAddress& address)
        {
            auto i = index.slots_by_wall.find(wall_key(address));
            return i != index.slots_by_wall.end()
                && std::ranges::any_of(i->second, [&](size_t slot) { return document.entities[slot].parent_id == parent; });
        };

        for (const std::vector<WallAddress>& walls : door_walls(document.cells))
        {
            std::set<stored_entity_id_type> parents;

            for (const WallAddress& address : walls)
            {
                auto i = index.slots_by_wall.find(wall_key(address));
                if (i == index.slots_by_wall.end())
                {
                    continue;
                }

                for (size_t slot : i->second)
                {
                    if (document.entities[slot].parent_id.has_value())
                    {
                        parents.insert(*document.entities[slot].parent_id);
                    }
                }
            }

            std::vector<stored_entity_id_type> doors;
            for (stored_entity_id_type parent : parents)
            {
                if (std::ranges::all_of(walls, [&](const WallAddress& address) { return child_on(parent, address); }))
                {
                    doors.push_back(parent);
                }
            }

            std::string text = walls_text(walls);

            if (doors.empty())
            {
                fail("entities", std::format("the doorway at {} has no entity with a child sited on each of its walls", text));
            }

            if (doors.size() > 1)
            {
                fail("entities", std::format("the doorway at {} has {} entities with a child sited on each of its walls", text, doors.size()));
            }

            for (size_t slot : index.slots_by_parent.find(doors.front())->second)
            {
                const WallAddress* address = wall_address(document.entities[slot]);

                if (address != nullptr && std::ranges::find(walls, *address) == walls.end())
                {
                    fail(element_path("entities", slot), std::format("sited on a wall that is not one of the walls of its door's doorway at {}", text));
                }
            }
        }

        return;
    }

    //--------------------------------------------------------------------------
    // The walls the open neighbors of each stair cell present to it, one of which is the staircase's open side
    std::map<discrete_point_type, std::vector<WallAddress>> stair_cell_walls(const Cells& cells)
    {
        std::map<discrete_point_type, std::vector<WallAddress>> result;

        for (coord_type j = 0; j < cells.extent.y(); ++j)
        {
            for (coord_type i = 0; i < cells.extent.x(); ++i)
            {
                discrete_point_type cell_position {i, j};
                if (!stair_cell(cells, cell_position))
                {
                    continue;
                }

                std::vector<WallAddress> walls;

                for (const auto& direction : quetzal::math::cardinal_directions<coord_type, 2>)
                {
                    discrete_point_type neighbor = cell_position + direction;
                    if (on_grid(cells, neighbor) && floor_door_cell(cells, neighbor) && has_wall(cells, neighbor, -direction))
                    {
                        walls.push_back({neighbor, direction});
                    }
                }

                result.emplace(cell_position, std::move(walls));
            }
        }

        return result;
    }

    //--------------------------------------------------------------------------
    // Called with the location and description of a discrepancy that is not an error
    using warn_type = std::function<void(const std::string& path, std::string_view what)>;

    //--------------------------------------------------------------------------
    // Each stair glyph is one entity with a relocator, sited on the wall on its open side. The glyphs are visual aids, so a
    // discrepancy is a warning
    void check_stair_glyphs(const Document& document, const EntityIndex& index,
        const std::map<discrete_point_type, std::vector<WallAddress>>& stair_walls, const warn_type& warn)
    {
        for (const auto& [cell_position, walls] : stair_walls)
        {
            std::vector<size_t> relocators;

            for (const WallAddress& address : walls)
            {
                auto i = index.slots_by_wall.find(wall_key(address));
                if (i == index.slots_by_wall.end())
                {
                    continue;
                }

                for (size_t slot : i->second)
                {
                    if (document.entities[slot].relocator.has_value())
                    {
                        relocators.push_back(slot);
                    }
                }
            }

            if (relocators.empty())
            {
                warn("entities", std::format("the stair cell at [{}, {}] has no entity with a relocator sited on a wall its open neighbors present to it",
                    cell_position.i(), cell_position.j()));
            }

            if (relocators.size() > 1)
            {
                warn(element_path("entities", relocators[1]), std::format("a second entity with a relocator on the walls around the stair cell at [{}, {}]",
                    cell_position.i(), cell_position.j()));
            }
        }

        return;
    }

    //--------------------------------------------------------------------------
    // A zone has at least one access point. Each is an entity with a relocator, sited on a wall, or on the floor or the
    // ceiling with an orientation. The relocator has a type; an order, counting from 0 without gaps among the access points of that type; and a
    // placement, the cell and orientation at which an arriving actor is placed. An access point on a wall that is not the open side of
    // a cell written '<' or '>' is a warning, since the glyphs are visual aids, and so is a relocator type that is neither the
    // entity's type nor its parent's
    void check_access_points(const Document& document, const EntityIndex& index,
        const std::map<discrete_point_type, std::vector<WallAddress>>& stair_walls, const warn_type& warn)
    {
        const std::string path = "layout.components.access_points";

        std::set<wall_key_type> open_sides;
        for (const auto& [cell_position, walls] : stair_walls)
        {
            for (const WallAddress& address : walls)
            {
                open_sides.insert(wall_key(address));
            }
        }

        std::set<wall_key_type> door_sides;
        for (const auto& walls : door_walls(document.cells))
        {
            for (const WallAddress& address : walls)
            {
                door_sides.insert(wall_key(address));
            }
        }

        // <type, <order, access point index>>
        std::map<std::string, std::map<std::uint64_t, size_t>> orders;

        for (size_t i = 0; i < document.access_points.size(); ++i)
        {
            std::string point_path = element_path(path, i);
            stored_entity_id_type id = document.access_points[i];

            auto slot = index.slots_by_id.find(id);
            if (slot == index.slots_by_id.end())
            {
                fail(point_path, std::format("refers to {}, which is no entity's id", id));
            }

            const Entity& entity = document.entities[slot->second];
            if (!entity.relocator.has_value())
            {
                fail(point_path, "an entity with no relocator");
            }

            const Relocator& relocator = *entity.relocator;

            if (!entity.site.has_value() || entity.site->relation != Relation::On || !entity.site->address.has_value())
            {
                fail(point_path, "an entity that is not sited on a surface: a wall, the floor or the ceiling");
            }

            const WallAddress* address = wall_address(entity);
            if (address == nullptr && !entity.site->orientation.has_value())
            {
                fail(member_path(member_path(element_path("entities", slot->second), "components"), "site"),
                    "an access point on the floor or the ceiling with no orientation");
            }

            if (address != nullptr && !open_sides.contains(wall_key(*address)) && !door_sides.contains(wall_key(*address)))
            {
                warn(point_path, "an entity sited on a wall that is neither the open side of a cell written '<' or '>' nor a door's wall");
            }

            std::string relocator_path = member_path(member_path(element_path("entities", slot->second), "components"), "relocator");

            if (!relocator.type.has_value())
            {
                fail(member_path(relocator_path, "type"), "required and absent");
            }

            // A door's access point is one of its door facings, so the relocator's type may be the parent's
            auto parent_slot = entity.parent_id.has_value() ? index.slots_by_id.find(*entity.parent_id) : index.slots_by_id.end();
            bool parent_type_matches = parent_slot != index.slots_by_id.end() && document.entities[parent_slot->second].type == *relocator.type;
            if (*relocator.type != entity.type && !parent_type_matches)
            {
                warn(member_path(relocator_path, "type"), std::format("the type '{}' is neither its entity's type '{}' nor its parent's",
                    *relocator.type, entity.type));
            }

            std::string order_path = member_path(relocator_path, "order");
            if (!relocator.order.has_value())
            {
                fail(order_path, "required and absent");
            }

            std::string placement_path = member_path(relocator_path, "placement");
            if (!relocator.placement.has_value())
            {
                fail(placement_path, "required and absent");
            }

            if (!floor_door_cell(document.cells, relocator.placement->position()))
            {
                fail(member_path(placement_path, "cell_position"), "a cell an actor cannot stand in");
            }

            if (!orders[*relocator.type].emplace(*relocator.order, i).second)
            {
                fail(order_path, std::format("order {} is another access point's of the type '{}'", *relocator.order, *relocator.type));
            }
        }

        for (const auto& [type, access_points_by_order] : orders)
        {
            auto last = std::prev(access_points_by_order.end());
            if (last->first != access_points_by_order.size() - 1)
            {
                fail(element_path(path, last->second), std::format("order {} is past a gap among the access points of the type '{}'",
                    last->first, type));
            }
        }

        return;
    }

    //--------------------------------------------------------------------------
    // The grid is read first, as every address refers to it
    Document read_zone_document(const raw_values_type& v, const warn_type& warn)
    {
        check_file_header(v, format_name, format_version);

        Document document;
        document.version = std::string(format_version);

        if (const raw_values_type* p = member(v, "zone"); p != nullptr)
        {
            object(*p, "zone");
            document.zone_id = id_value(required_member(*p, "zone", "id"), "zone.id");
        }

        if (const raw_values_type* p = member(v, "generator"); p != nullptr)
        {
            document.generator = object(*p, "generator");
        }

        if (const raw_values_type* p = member(v, "layout"); p != nullptr)
        {
            object(*p, "layout");
            document.components = read_components(*p, "layout");
        }

        document.access_points = read_access_points(document.components);
        document.level = read_level(document.components);
        document.is_unpopulated = read_unpopulated(document.components);
        if (document.components.is_object())
        {
            document.components.erase("access_points");
            document.components.erase("level");
            document.components.erase("unpopulated");
        }

        if (document.is_unpopulated && document.zone_id.has_value())
        {
            warn("layout.components.unpopulated", "set in a zone with a zone id");
        }

        document.cells = read_cells(required_member(v, "", "cells"), "cells");

        if (const raw_values_type* p = member(v, "barriers"); p != nullptr)
        {
            document.barriers = read_barriers(*p, "barriers", document.cells);
        }

        if (const raw_values_type* p = member(v, "rooms"); p != nullptr)
        {
            const raw_values_type& a = array(*p, "rooms");
            document.rooms.reserve(a.size());

            for (size_t i = 0; i < a.size(); ++i)
            {
                document.rooms.push_back(read_room(a[i], element_path("rooms", i), document.cells));
            }

            check_rooms(document);
        }

        if (const raw_values_type* p = member(v, "walls"); p != nullptr)
        {
            const raw_values_type& a = array(*p, "walls");
            document.walls.reserve(a.size());

            std::set<std::array<coord_type, 4>> addresses;

            for (size_t i = 0; i < a.size(); ++i)
            {
                Wall wall = read_wall(a[i], element_path("walls", i), document.cells);

                const WallAddress& address = wall.address;
                if (!addresses.insert({address.cell_position.i(), address.cell_position.j(), address.normal.x(), address.normal.y()}).second)
                {
                    fail(member_path(element_path("walls", i), "wall"), "a second entry for one wall");
                }

                document.walls.push_back(std::move(wall));
            }
        }

        if (const raw_values_type* p = member(v, "flats"); p != nullptr)
        {
            const raw_values_type& a = array(*p, "flats");
            document.flats.reserve(a.size());

            for (size_t i = 0; i < a.size(); ++i)
            {
                Flat flat = read_flat(a[i], element_path("flats", i), document.cells, document.rooms.size());

                for (const Flat& other : document.flats)
                {
                    if (other.role == flat.role)
                    {
                        fail(element_path("flats", i), "a second flat in one role");
                    }
                }

                document.flats.push_back(std::move(flat));
            }
        }

        if (const raw_values_type* p = member(v, "entities"); p != nullptr)
        {
            const raw_values_type& a = array(*p, "entities");
            document.entities.reserve(a.size());

            std::set<stored_entity_id_type> ids;

            for (size_t i = 0; i < a.size(); ++i)
            {
                std::string entity_path = element_path("entities", i);
                Entity entity = read_entity(a[i], entity_path, document.cells);

                if (!ids.insert(entity.id).second)
                {
                    fail(member_path(entity_path, "id"), std::format("a second entity with the id {}", entity.id));
                }

                document.entities.push_back(std::move(entity));
            }
        }

        EntityIndex index = entity_index(document.entities);
        check_entities_placed(document.entities, index);
        check_entities_acyclic(document.entities, index);
        check_references(document, index);

        std::map<discrete_point_type, std::vector<WallAddress>> stair_walls = stair_cell_walls(document.cells);
        check_door_glyphs(document, index);
        check_stair_glyphs(document, index, stair_walls, warn);
        check_access_points(document, index, stair_walls, warn);

        return document;
    }

} // namespace

//------------------------------------------------------------------------------
quetzal::stratum_format::Document quetzal::stratum_format::read_zone(std::istream& is, const std::string& stream_name)
{
    return detail::read_document<Document>(is, stream_name, [&](const raw_values_type& v)
    {
        return read_zone_document(v, [&](const std::string& path, std::string_view what)
        {
            log::process_log().warning(log_category, "{}: {}: {}", stream_name, path, what);
        });
    });
}

//------------------------------------------------------------------------------
quetzal::stratum_format::Document quetzal::stratum_format::read_zone(const std::filesystem::path& filepath)
{
    std::ifstream is(filepath, std::ios::binary);
    if (!is)
    {
        throw std::runtime_error(std::format("{}: cannot be opened", filepath.string()));
    }

    return read_zone(is, filepath.string());
}
