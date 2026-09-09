#if !defined(QUETZAL_STRATUM_SIM_FLAT_HPP)
#define QUETZAL_STRATUM_SIM_FLAT_HPP
//------------------------------------------------------------------------------
// stratum_sim
// Flat.hpp
//------------------------------------------------------------------------------

#include "GridMapping.hpp"
#include "types.hpp"
#include <optional>
#include <vector>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // A horizontal surface bounding the layout, one below and one above. The two differ by
    // role and nothing else, a stalactite and a stalagmite being one shape inverted
    // A height field rather than a plane. A constant height is the ordinary case and is what
    // a level with no height data loads, so it costs one value and no table; sampling is
    // what a sloped floor, an arched ceiling or a flight of steps needs, and the two are one
    // record with more or fewer of its features used
    // Sampling is over a grid of the surface's own, which need not be the movement grid: a
    // height map may be finer without anything else noticing. See the three grids in
    // sim_zone_model.md
    class Flat
    {
    public:

        using heights_type = std::vector<std::optional<value_type>>;

        Flat() = default;
        explicit Flat(value_type height, bool ceiling = false);

        bool ceiling() const; // Role, floor otherwise
        void set_ceiling(bool b);

        // Where the surface is at a position in the zone, and nothing where it is absent. A
        // pit, a shaft, and a hole dug through the floor are all somewhere the surface is not,
        // rather than somewhere it is low
        // A position off the sampling grid is off the surface and returns nothing
        std::optional<value_type> height(const point_type& position) const;

        // The constant the surface takes wherever it is not sampled, which is everywhere
        // until something is
        value_type height() const;
        void set_height(value_type height);

        // Gives the surface a sampling grid, every sample present and at the constant height.
        // Sampling again replaces what was there
        void set_sampling(const GridMapping& grid);
        bool sampled() const;
        const GridMapping& sampling() const; // Must be sampled

        // One sample, which must lie on the sampling grid. Nothing where the surface is absent
        std::optional<value_type> sample(const discrete_point_type& grid_position) const;
        void set_sample(const discrete_point_type& grid_position, std::optional<value_type> height);

    private:

        value_type m_height = value_type(0);
        bool m_ceiling = false;
        std::optional<GridMapping> m_sampling;
        heights_type m_heights; // The sampling grid's extent, empty until it has one
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_FLAT_HPP
