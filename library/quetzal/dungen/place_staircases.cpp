//------------------------------------------------------------------------------
// dungen
// place_staircases.cpp
//------------------------------------------------------------------------------

#include "place_staircases.hpp"
#include <algorithm>
#include <stdexcept>

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

    for (size_t i = 0; i < count; ++i)
    {
        level.create_staircase(candidates[i], i < up_count ? StaircaseDirection::Up : StaircaseDirection::Down);
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

        for (const auto& direction : cardinal_directions)
        {
            point_type neighbor = position + direction;
            if (!level.cells().contains(neighbor))
            {
                continue;
            }

            const Cell& cell = level.cell(neighbor);
            if ((cell.type() == CellType::Room || cell.type() == CellType::Corridor) && space_of(cell) == space
                && !doorway(level, neighbor) && !blocks_doorway(level, neighbor))
            {
                available.push_back(neighbor);
            }
        }

        if (available.empty())
        {
            continue;
        }

        point_type destination = available[random.gen(available.size())];

        level.cell(position).set_type(space == nullid ? CellType::Corridor : CellType::Room);
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
            if (floor_staircase(type))
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