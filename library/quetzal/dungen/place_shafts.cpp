//------------------------------------------------------------------------------
// dungen
// place_shafts.cpp
//------------------------------------------------------------------------------

#include "place_shafts.hpp"
#include <algorithm>

using namespace std;

namespace
{

    using namespace quetzal::dungen;

    //--------------------------------------------------------------------------
    size_t shaft_count(const Level& level, ShaftPlacements placements, random_type& random)
    {
        if (placements == ShaftPlacements::Paired)
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
void quetzal::dungen::place_shafts(Level& level, vector<position_type> candidates, ShaftPlacements placements, random_type& random)
{
    if (placements == ShaftPlacements::None)
    {
        return;
    }

    shuffle(candidates.begin(), candidates.end(), random.engine());

    size_t n_shafts = min(shaft_count(level, placements, random), candidates.size());

    for (size_t i = 0; i < n_shafts; ++i)
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

        level.create_shaft(candidates[i]);
//        level.shafts().back() set property down/up
    }

    return;
}
