//------------------------------------------------------------------------------
// dungen
// place_staircases.cpp
//------------------------------------------------------------------------------

#include "place_staircases.hpp"
#include <algorithm>
#include <stdexcept>
#include <cassert>

using namespace std;

namespace
{

    using namespace quetzal::dungen;
    using quetzal::id_type;
    using quetzal::nullid;

    //--------------------------------------------------------------------------
    // A range of one draws nothing, so a fixed count leaves every draw after it where it was
    size_t draw_count(const quetzal::math::Interval<size_t>& count, random_type& random)
    {
        return count.lower() == count.upper() ? count.lower() : random.gen(count.lower(), count.upper());
    }

    //--------------------------------------------------------------------------
    bool floor_staircase(CellType type)
    {
        return type == CellType::Room || type == CellType::Corridor || type == CellType::Stair;
    }

    //--------------------------------------------------------------------------
    // The room a floor/staircase cell is in, nullid for corridor
    id_type space_of(const Cell& cell)
    {
        return cell.type() == CellType::Room || cell.type() == CellType::Stair ? cell.room_id() : nullid;
    }

    //--------------------------------------------------------------------------
    bool doorway(const Level& level, point_type position)
    {
        const Cell& cell = level.cell(position);

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
            if (level.cells().contains(neighbor) && floor_staircase(level.cell(neighbor).type()) && space_of(level.cell(neighbor)) != nullid)
            {
                return true;
            }
        }

        return false;
    }

    //--------------------------------------------------------------------------
    bool blocks_doorway(const Level& level, point_type position)
    {
        for (const auto& direction : cardinal_directions)
        {
            point_type neighbor = position + direction;
            if (level.cells().contains(neighbor) && doorway(level, neighbor))
            {
                return true;
            }
        }

        return false;
    }

    //--------------------------------------------------------------------------
    // A door counts, being passable once opened
    bool passable_at(const Level& level, point_type position)
    {
        if (!level.cells().contains(position))
        {
            return false;
        }

        CellType type = level.cell(position).type();
        return type == CellType::Room || type == CellType::Corridor || type == CellType::Door;
    }

    //--------------------------------------------------------------------------
    // Whether a staircase's block at a passable position leaves what staircases need: a passable side of its own to face,
    // one still beside each staircase next to it, and its passable neighbors still reaching one another around it
    bool block_allowed(const Level& level, point_type position)
    {
        assert(passable_at(level, position));

        auto has_open_side = [&](point_type cell)
        {
            return any_of(cardinal_directions.begin(), cardinal_directions.end(), [&](const vector_type& direction)
            {
                point_type neighbor = cell + direction;
                return neighbor != position && passable_at(level, neighbor);
            });
        };

        vector<point_type> sides;

        for (const auto& direction : cardinal_directions)
        {
            point_type neighbor = position + direction;
            if (passable_at(level, neighbor))
            {
                sides.push_back(neighbor);
            }
            else if (level.cells().contains(neighbor) && level.cell(neighbor).type() == CellType::Stair && !has_open_side(neighbor))
            {
                return false;
            }
        }

        if (sides.empty())
        {
            return false;
        }

        auto [nx, ny] = level.size().rep();
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
                if (neighbor == position || !passable_at(level, neighbor) || reached[slot(neighbor)])
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

} // namespace

//------------------------------------------------------------------------------
void quetzal::dungen::place_staircases(Level& level, vector<point_type> candidates, const math::Interval<size_t>& staircase_down_count,
    const math::Interval<size_t>& staircase_up_count, random_type& random)
{
    size_t down_count = draw_count(staircase_down_count, random);
    size_t up_count = draw_count(staircase_up_count, random);
    size_t count = min(down_count + up_count, candidates.size());

    if (count == 0)
    {
        throw runtime_error("the level has no cell a staircase can take, and so no entry");
    }

    shuffle(candidates.begin(), candidates.end(), random.engine());

    size_t placed = 0;

    for (size_t i = 0; i < candidates.size() && placed < count; ++i)
    {
        if (block_allowed(level, candidates[i]))
        {
            level.create_staircase(candidates[i], placed < up_count ? StaircaseDirection::Up : StaircaseDirection::Down);
            ++placed;
        }
    }

    if (placed == 0)
    {
        throw runtime_error("the level has no cell a staircase can take without closing off a passage, and so no entry");
    }

    return;
}

//------------------------------------------------------------------------------
void quetzal::dungen::move_staircases_off_doorways(Level& level, random_type& random)
{
    for (auto& staircase : level.staircases())
    {
        point_type position = staircase.position();

        if (!blocks_doorway(level, position))
        {
            continue;
        }

        id_type space = space_of(level.cell(position));
        vector<point_type> available;

        // Floor again while its destination is chosen, so the block there is judged without this one
        level.cell(position).set_type(space == nullid ? CellType::Corridor : CellType::Room);

        for (const auto& direction : cardinal_directions)
        {
            point_type neighbor = position + direction;
            if (!level.cells().contains(neighbor))
            {
                continue;
            }

            const Cell& cell = level.cell(neighbor);
            if ((cell.type() == CellType::Room || cell.type() == CellType::Corridor) && space_of(cell) == space
                && !doorway(level, neighbor) && !blocks_doorway(level, neighbor) && block_allowed(level, neighbor))
            {
                available.push_back(neighbor);
            }
        }

        if (available.empty())
        {
            level.cell(position).set_type(CellType::Stair);
            continue;
        }

        point_type destination = available[random.gen(available.size())];

        level.cell(destination).set_type(CellType::Stair);
        staircase.set_position(destination);
    }

    return;
}

//------------------------------------------------------------------------------
void quetzal::dungen::face_staircases(Level& level, random_type& random)
{
    for (auto& staircase : level.staircases())
    {
        vector<vector_type> open;
        vector<vector_type> doors;

        for (const auto& direction : cardinal_directions)
        {
            point_type neighbor = staircase.position() + direction;
            if (!level.cells().contains(neighbor))
            {
                continue;
            }

            CellType type = level.cell(neighbor).type();
            if (type == CellType::Room || type == CellType::Corridor)
            {
                open.push_back(direction);
            }
            else if (type == CellType::Door)
            {
                doors.push_back(direction);
            }
        }

        const vector<vector_type>& sides = open.empty() ? doors : open;
        if (!sides.empty())
        {
            staircase.set_facing(sides[random.gen(sides.size())]);
        }
    }

    return;
}
