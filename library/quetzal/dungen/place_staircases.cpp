//------------------------------------------------------------------------------
// dungen
// place_staircases.cpp
//------------------------------------------------------------------------------

#include "place_staircases.hpp"
#include "quetzal/log/Log.hpp"
#include <algorithm>
#include <map>
#include <stdexcept>
#include <tuple>
#include <cassert>

using namespace std;

namespace
{

    using namespace quetzal::dungen;
    using quetzal::id_type;
    using quetzal::nullid;

    //--------------------------------------------------------------------------
    // A range of one value draws nothing, so a fixed count leaves every later draw unchanged
    size_t draw_count(const quetzal::math::Interval<size_t>& count, random_type& random)
    {
        return count.lower() == count.upper() ? count.lower() : random.random(count.lower(), count.upper());
    }

    //--------------------------------------------------------------------------
    bool floor_staircase(CellType type)
    {
        return type == CellType::Room || type == CellType::Corridor || type == CellType::Staircase;
    }

    //--------------------------------------------------------------------------
    // The room a floor/staircase cell is in, nullid for corridor
    id_type room(const Cell& cell)
    {
        return cell.type() == CellType::Room || cell.type() == CellType::Staircase ? cell.room_id() : nullid;
    }

    //--------------------------------------------------------------------------
    bool doorway(const Zone& zone, point_type position)
    {
        const Cell& cell = zone.cell(position);

        if (cell.type() == CellType::Door)
        {
            return true;
        }

        if (cell.type() != CellType::Corridor)
        {
            return false;
        }

        for (const auto& direction : cardinal_directions)
        {
            point_type neighbor = position + direction;
            if (zone.cells().contains(neighbor) && floor_staircase(zone.cell(neighbor).type()) && room(zone.cell(neighbor)) != nullid)
            {
                return true;
            }
        }

        return false;
    }

    //--------------------------------------------------------------------------
    bool blocks_doorway(const Zone& zone, point_type position)
    {
        for (const auto& direction : cardinal_directions)
        {
            point_type neighbor = position + direction;
            if (zone.cells().contains(neighbor) && doorway(zone, neighbor))
            {
                return true;
            }
        }

        return false;
    }

    //--------------------------------------------------------------------------
    // A door counts, since it is passable once opened
    bool passable_at(const Zone& zone, point_type position)
    {
        if (!zone.cells().contains(position))
        {
            return false;
        }

        CellType type = zone.cell(position).type();
        return type == CellType::Room || type == CellType::Corridor || type == CellType::Door;
    }

    //--------------------------------------------------------------------------
    // Returns true if a staircase's block can go at a passable position. The block needs a passable side to face, each
    // staircase next to it must keep a passable side, and the block's passable neighbors must still reach one another around it
    bool block_allowed(const Zone& zone, point_type position)
    {
        assert(passable_at(zone, position));

        auto has_open_side = [&](point_type cell)
        {
            return any_of(cardinal_directions.begin(), cardinal_directions.end(), [&](const vector_type& direction)
            {
                point_type neighbor = cell + direction;
                return neighbor != position && passable_at(zone, neighbor);
            });
        };

        vector<point_type> sides;

        for (const auto& direction : cardinal_directions)
        {
            point_type neighbor = position + direction;
            if (passable_at(zone, neighbor))
            {
                sides.push_back(neighbor);
            }
            else if (zone.cells().contains(neighbor) && zone.cell(neighbor).type() == CellType::Staircase && !has_open_side(neighbor))
            {
                return false;
            }
        }

        if (sides.empty())
        {
            return false;
        }

        auto [nx, ny] = zone.extent().rep();
        auto slot = [nx](point_type cell) { return static_cast<size_t>(cell.y()) * static_cast<size_t>(nx) + static_cast<size_t>(cell.x()); };

        vector<bool> reached(static_cast<size_t>(nx) * static_cast<size_t>(ny), false);
        reached[slot(sides.front())] = true;

        vector<point_type> pending {sides.front()};
        size_t sides_unreached = sides.size() - 1;

        while (!pending.empty() && sides_unreached > 0)
        {
            point_type cell = pending.back();
            pending.pop_back();

            for (const auto& direction : cardinal_directions)
            {
                point_type neighbor = cell + direction;
                if (neighbor == position || !passable_at(zone, neighbor) || reached[slot(neighbor)])
                {
                    continue;
                }

                reached[slot(neighbor)] = true;
                pending.push_back(neighbor);

                if (find(sides.begin(), sides.end(), neighbor) != sides.end())
                {
                    --sides_unreached;
                }
            }
        }

        return sides_unreached == 0;
    }

    //--------------------------------------------------------------------------
    // Returns a staircase at the cell, facing a random side that has floor beyond it. Returns none if the cell is not floor,
    // is a doorway or beside one, is the floor another staircase faces, would cut a passage in two, or has no floor beside
    // it
    std::optional<directed_point_type> staircase_placement(const Zone& zone, point_type cell_position, random_type& random)
    {
        auto floor = [&](point_type position)
        {
            CellType type = zone.cell(position).type();
            return type == CellType::Room || type == CellType::Corridor;
        };

        auto faced = any_of(zone.staircases().begin(), zone.staircases().end(), [&](const Staircase& staircase)
        {
            return staircase.position() + staircase.direction() == cell_position;
        });

        if (!floor(cell_position) || faced || doorway(zone, cell_position) || blocks_doorway(zone, cell_position)
            || !block_allowed(zone, cell_position))
        {
            return std::nullopt;
        }

        vector<vector_type> open_sides;

        for (const auto& direction : cardinal_directions)
        {
            point_type neighbor = cell_position + direction;
            if (zone.cells().contains(neighbor) && floor(neighbor))
            {
                open_sides.push_back(direction);
            }
        }

        if (open_sides.empty())
        {
            return std::nullopt;
        }

        return directed_point_type(cell_position, random.choose(open_sides));
    }

} // namespace

//------------------------------------------------------------------------------
void quetzal::dungen::place_staircases(Zone& zone, vector<point_type> candidates, const math::Interval<size_t>& up_interval,
    const math::Interval<size_t>& down_interval, random_type& random)
{
    size_t down_count = draw_count(down_interval, random);
    size_t up_count = draw_count(up_interval, random);
    const size_t down_requested = down_count;
    const size_t up_requested = up_count;

    if ((up_count > 0 || down_count > 0) && candidates.size() == 0)
    {
        throw runtime_error("the zone has no place for a staircase");
    }

    random.shuffle(candidates);

    auto rise = up_count > 0 ? Rise::Up : Rise::Down;
    // 0 requested means automatic success
    bool up_placed = up_count == 0;
    bool down_placed = down_count == 0;

    using counter_type = std::tuple<bool&, size_t&, size_t&, Rise>;
    std::map<Rise, counter_type> counters =
    {
        {Rise::Up, {up_placed, up_count, down_count, Rise::Down}},
        {Rise::Down, {down_placed, down_count, up_count, Rise::Up}},
    };

    for (size_t i = 0; i < candidates.size(); ++i)
    {
        auto directed_position = staircase_placement(zone, candidates[i], random);
        if (!directed_position)
        {
            continue;
        }

        zone.create_staircase(to_string(rise), *directed_position, rise);
        auto [placed, count, count_other, rise_other] = counters.at(rise);
        assert(count > 0);
        --count;
        placed = true;
        if (count_other > 0)
        {
            rise = rise_other;
        }

        if (up_count == 0 && down_count == 0)
        {
            break;
        }
    }

    if (!up_placed || !down_placed)
    {
        throw runtime_error("at least one of each requested staircase rise could not be placed");
    }
    else if (up_count > 0 || down_count > 0)
    {
        log::logger().warning(log_category, "{} of {} staircases leading up and {} of {} leading down placed, the candidates running out",
            up_requested - up_count, up_requested, down_requested - down_count, down_requested);
    }

    return;
}
