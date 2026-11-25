//------------------------------------------------------------------------------
// stratum_view
// ascii.cpp
//------------------------------------------------------------------------------

#include "ascii.hpp"

namespace sim = quetzal::stratum_sim;

namespace
{

    using quetzal::stratum_view::Glyph;
    using quetzal::stratum_view::Glyphs;
    using quetzal::stratum_view::FeatureType;
    using quetzal::stratum_view::DirectionMask;
    using quetzal::stratum_view::Place;
    using sim::AccessState;
    using quetzal::stratum_core::Directionality;

    //--------------------------------------------------------------------------
    // An access, a locked state, a detection, a directionality, a mask, and an actor add one each
    int specificity(const Glyph& glyph)
    {
        return (glyph.access.has_value() ? 1 : 0) + (glyph.locked.has_value() ? 1 : 0) + (glyph.detection.has_value() ? 1 : 0)
            + (glyph.directionality.has_value() ? 1 : 0) + (glyph.mask.has_value() ? 1 : 0) + (glyph.actor ? 1 : 0);
    }

    //--------------------------------------------------------------------------
    bool matches(const Glyph& glyph, const Place& place)
    {
        return glyph.type == place.type
            && (!glyph.access.has_value() || glyph.access == place.access)
            && (!glyph.locked.has_value() || glyph.locked == place.locked)
            && (!glyph.detection.has_value() || *glyph.detection == place.detection)
            && (!glyph.directionality.has_value() || glyph.directionality == place.directionality)
            && (!glyph.mask.has_value() || *glyph.mask == place.mask)
            && (!glyph.actor || place.actor);
    }

    //--------------------------------------------------------------------------
    // Adds a line for each of the sixteen masks: horizontal, vertical, a corner if wall lines reach the position along both
    // axes, and an opening if none reach it
    void add_walls(std::vector<Glyph>& glyphs, char horizontal, char vertical, char corner, char opening)
    {
        for (uint8_t bits = 0; bits < 16; ++bits)
        {
            DirectionMask mask = static_cast<DirectionMask>(bits);

            bool across = any(mask & (DirectionMask::East | DirectionMask::West));
            bool along = any(mask & (DirectionMask::North | DirectionMask::South));

            glyphs.push_back(Glyph{.type = FeatureType::Wall, .mask = mask,
                .c = across && along ? corner : across ? horizontal : along ? vertical : opening});
        }

        return;
    }

} // namespace

//------------------------------------------------------------------------------
quetzal::stratum_view::Glyphs::Glyphs(std::vector<Glyph> glyphs) :
    m_glyphs(std::move(glyphs))
{
}

//------------------------------------------------------------------------------
char quetzal::stratum_view::Glyphs::operator()(const Place& place) const
{
    const Glyph* best = nullptr;
    int best_specificity = -1;

    for (const Glyph& glyph : m_glyphs)
    {
        int n = specificity(glyph);

        if (n > best_specificity && matches(glyph, place))
        {
            best = &glyph;
            best_specificity = n;
        }
    }

    return best != nullptr ? best->c : ' ';
}

//------------------------------------------------------------------------------
const std::vector<quetzal::stratum_view::Glyph>& quetzal::stratum_view::Glyphs::glyphs() const
{
    return m_glyphs;
}

//------------------------------------------------------------------------------
const quetzal::stratum_view::Glyphs& quetzal::stratum_view::cell_glyphs()
{
    static const Glyphs glyphs(std::vector<Glyph>
    {
        {.type = FeatureType::Floor, .actor = true, .c = '@'},
        {.type = FeatureType::Door, .access = AccessState::Closed, .actor = true, .c = '@'},
        {.type = FeatureType::Door, .access = AccessState::Open, .actor = true, .c = '@'},
        {.type = FeatureType::Staircase, .directionality = Directionality::Up, .actor = true, .c = '@'},
        {.type = FeatureType::Staircase, .directionality = Directionality::Down, .actor = true, .c = '@'},
        {.type = FeatureType::Solid, .c = '#'},
        {.type = FeatureType::Floor, .c = '.'},
        {.type = FeatureType::Door, .access = AccessState::Closed, .c = '/'},
        {.type = FeatureType::Door, .access = AccessState::Open, .c = '\\'},
        {.type = FeatureType::Staircase, .directionality = Directionality::Up, .c = '<'},
        {.type = FeatureType::Staircase, .directionality = Directionality::Down, .c = '>'}
    });

    return glyphs;
}

//------------------------------------------------------------------------------
const quetzal::stratum_view::Glyphs& quetzal::stratum_view::lattice_glyphs()
{
    static const Glyphs glyphs = []
    {
        std::vector<Glyph> v
        {
            {.type = FeatureType::Floor, .actor = true, .c = '@'},
            {.type = FeatureType::Door, .access = AccessState::Closed, .actor = true, .c = '@'},
            {.type = FeatureType::Door, .access = AccessState::Open, .actor = true, .c = '@'},
            {.type = FeatureType::Staircase, .directionality = Directionality::Up, .actor = true, .c = '@'},
            {.type = FeatureType::Staircase, .directionality = Directionality::Down, .actor = true, .c = '@'},
            {.type = FeatureType::Solid, .c = '#'},
            {.type = FeatureType::None, .c = ' '},
            {.type = FeatureType::Floor, .c = ' '},
            {.type = FeatureType::Door, .access = AccessState::Closed, .c = '/'},
            {.type = FeatureType::Door, .access = AccessState::Open, .c = '\\'},
            {.type = FeatureType::Staircase, .directionality = Directionality::Up, .c = '<'},
            {.type = FeatureType::Staircase, .directionality = Directionality::Down, .c = '>'}
        };

        add_walls(v, '-', '|', '+', ' ');
        return Glyphs(std::move(v));
    }();

    return glyphs;
}

//------------------------------------------------------------------------------
quetzal::stratum_view::Console quetzal::stratum_view::render(const Cells& cells, const Glyphs& glyphs)
{
    const discrete_vector2_type& extent = cells.extent();
    Console console(extent, ' ');

    for (coord_type j = 0; j < extent.y(); ++j)
    {
        for (coord_type i = 0; i < extent.x(); ++i)
        {
            console.set({i, j}, glyphs(cells.at({i, j})));
        }
    }

    return console;
}

//------------------------------------------------------------------------------
quetzal::stratum_view::Console quetzal::stratum_view::render(const sim::World& world, flecs::entity zone, flecs::entity viewer, const Glyphs& glyphs)
{
    return render(cells(world, zone, viewer), glyphs);
}

//------------------------------------------------------------------------------
quetzal::stratum_view::Console quetzal::stratum_view::render_lattice(const Lattice& lattice, const Glyphs& glyphs)
{
    const discrete_vector2_type& extent = lattice.extent();
    Console console(extent, ' ');

    for (coord_type j = 0; j < extent.y(); ++j)
    {
        for (coord_type i = 0; i < extent.x(); ++i)
        {
            console.set({i, j}, glyphs(lattice.at({i, j})));
        }
    }

    return console;
}

//------------------------------------------------------------------------------
quetzal::stratum_view::Console quetzal::stratum_view::render_lattice(const sim::World& world, flecs::entity zone, flecs::entity viewer,
    const Glyphs& glyphs)
{
    return render_lattice(lattice(world, zone, viewer), glyphs);
}
