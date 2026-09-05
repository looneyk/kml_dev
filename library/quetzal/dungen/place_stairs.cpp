//------------------------------------------------------------------------------
// dungen
// place_stairs.cpp
//------------------------------------------------------------------------------

#include "place_stairs.hpp"
#include <algorithm>

using namespace std;

namespace
{

    using namespace quetzal::dungen;

    //--------------------------------------------------------------------------
    size_t stair_count(const Level& level, StairPlacements placements, random_type& random)
    {
        if (placements == StairPlacements::Paired)
        {
            return 2;
        }

        auto [nx, ny] = level.size();

        // At least one to draw from; a level below a thousand cells otherwise leaves an
        // empty range
        coord_type range = max<coord_type>(1, nx * ny / 1000);
        return 3 + static_cast<size_t>(random.gen(range));
    }

} // namespace

//------------------------------------------------------------------------------
void quetzal::dungen::place_stairs(Level& level, vector<position_type> candidates, StairPlacements placements, random_type& random)
{
    if (placements == StairPlacements::None)
    {
        return;
    }

    shuffle(candidates.begin(), candidates.end(), random.engine());

    size_t n_stairs = min(stair_count(level, placements, random), candidates.size());

    for (size_t i = 0; i < n_stairs; ++i)
    {
        bool down = true;
        if (i == 1)
        {
            down = false;
        }
        else if (i > 1)
        {
            down = random.gen(2) == 0;
        }

        level.create_stair(candidates[i]);
//        level.stairs().back() set property down/up
    }

    return;
}
