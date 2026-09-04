//------------------------------------------------------------------------------
// dungen
// write_level.cpp
//------------------------------------------------------------------------------

#include "write_level.hpp"
#include <ostream>
#include <cassert>

using namespace std;

namespace
{

    using quetzal::dungen::Parameters;

    //--------------------------------------------------------------------------
    void write_maze1(ostream& os, const Parameters& parameters)
    {
        const auto& maze1 = parameters.maze1;

        os << " room_dimension_base=" << maze1.room_dimension_base
            << " room_dimension_variation=" << maze1.room_dimension_variation
            << " room_density=" << maze1.room_density
            << " room_merge=" << (maze1.room_merge ? "true" : "false")
            << " corridor_straightness=" << maze1.corridor_straightness
            << " probability_dead_end_removal=" << maze1.probability_dead_end_removal
            << " probability_loop_removal=" << maze1.probability_loop_removal;
        return;
    }

    //--------------------------------------------------------------------------
    void write_grid1(ostream& os, const Parameters& parameters)
    {
        const auto& grid1 = parameters.grid1;

        os << " divisions=" << grid1.divisions.x() << "," << grid1.divisions.y()
            << " room_density=" << grid1.room_density
            << " room_dimension_min=" << grid1.room_dimension_min
            << " room_margin=" << grid1.room_margin
            << " loop=" << grid1.loop;
        return;
    }

    //--------------------------------------------------------------------------
    void write_cave1(ostream& os, const Parameters& parameters)
    {
        const auto& cave1 = parameters.cave1;

        os << " fill=" << cave1.fill
            << " smoothing_passes=" << cave1.smoothing_passes
            << " birth_threshold=" << cave1.birth_threshold
            << " survival_threshold=" << cave1.survival_threshold;
        return;
    }

} // namespace

//------------------------------------------------------------------------------
void quetzal::dungen::write_level(ostream& os, const Level& level, const Parameters& parameters)
{
    // The level's own size rather than the requested one, an algorithm being free to round
    // what it was given to something its layout can use
    auto [nx, ny] = level.size();

    os << "dungen " << format_version
        << " algorithm=" << parameters.algorithm
        << " size=" << nx << "," << ny
        << " shaft_placements=" << parameters.shaft_placements;

    switch (parameters.algorithm)
    {
        case Algorithm::Maze1:
            write_maze1(os, parameters);
            break;

        case Algorithm::Grid1:
            write_grid1(os, parameters);
            break;

        case Algorithm::Cave1:
            write_cave1(os, parameters);
            break;

        default:
            assert(false);
    }

    os << " seed=" << parameters.seed.value() << endl; // Throws rather than write a header that cannot reproduce

    level.render(os);
    return;
}
