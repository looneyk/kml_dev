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
        return count.length() == 0 ? count.lower() : random.random_closed(count.lower(), count.upper());
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
    // A door counts, since it is passable once opened, and so does a staircase, which is on the floor of its cell
    bool passable_at(const Zone& zone, point_type position)
    {
        if (!zone.cells().contains(position))
        {
            return false;
        }

        CellType type = zone.cell(position).type();
        return type == CellType::Room || type == CellType::Corridor || type == CellType::Door || type == CellType::Staircase;
    }

    //--------------------------------------------------------------------------
    bool beside_doorway(const Zone& zone, point_type position)
    {
        return any_of(cardinal_directions.begin(), cardinal_directions.end(), [&](const vector_type& direction)
        {
            point_type neighbor = position + direction;
            return zone.cells().contains(neighbor) && doorway(zone, neighbor);
        });
    }

    //--------------------------------------------------------------------------
    // Returns a staircase on the floor of the cell, oriented toward a random passable side. Returns none if the cell is not
    // floor, is a doorway, or is beside one
    std::optional<directed_point_type> staircase_placement(const Zone& zone, point_type cell_position, random_type& random)
    {
        CellType type = zone.cell(cell_position).type();
        if ((type != CellType::Room && type != CellType::Corridor) || doorway(zone, cell_position) || beside_doorway(zone, cell_position))
        {
            return std::nullopt;
        }

        vector<vector_type> passable_sides;

        for (const auto& direction : cardinal_directions)
        {
            if (passable_at(zone, cell_position + direction))
            {
                passable_sides.push_back(direction);
            }
        }

        if (passable_sides.empty())
        {
            passable_sides.assign(cardinal_directions.begin(), cardinal_directions.end());
        }

        return directed_point_type(cell_position, random.choose(passable_sides));
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
        auto placement = staircase_placement(zone, candidates[i], random);
        if (!placement)
        {
            continue;
        }

        zone.create_staircase(to_string(rise), *placement, rise);
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
