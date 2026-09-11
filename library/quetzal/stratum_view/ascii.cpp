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
    using quetzal::stratum_view::PlaceType;
    using quetzal::stratum_view::DirectionMask;
    using quetzal::stratum_view::Place;
    using quetzal::stratum_view::State;
    using quetzal::stratum_view::any;

    //--------------------------------------------------------------------------
    // How much of a place a line specifies, a state flag, a mask and an actor counting one
    // each. A line specifying more of it is preferred to one specifying less
    int specificity(const Glyph& glyph)
    {
        int n = (glyph.mask.has_value() ? 1 : 0) + (glyph.actor ? 1 : 0);

        for (uint16_t bit = 1; bit != 0; bit = static_cast<uint16_t>(bit << 1))
        {
            n += any(glyph.state & static_cast<State>(bit)) ? 1 : 0;
        }

        return n;
    }

    //--------------------------------------------------------------------------
    bool matches(const Glyph& glyph, const Place& place)
    {
        return glyph.type == place.type
            && (glyph.state & place.state) == glyph.state
            && (!glyph.mask.has_value() || *glyph.mask == place.mask)
            && (!glyph.actor || place.actor);
    }

    //--------------------------------------------------------------------------
    // ASCII has three glyphs for the sixteen joints the mask tells apart, so a wall is
    // sixteen lines: a run passing through horizontally or vertically, a junction where both
    // axes reach it, and nothing where neither does
    void add_walls(std::vector<Glyph>& glyphs, char horizontal, char vertical, char corner, char opening)
    {
        for (uint8_t bits = 0; bits < 16; ++bits)
        {
            DirectionMask mask = static_cast<DirectionMask>(bits);

            bool across = any(mask & (DirectionMask::East | DirectionMask::West));
            bool along = any(mask & (DirectionMask::North | DirectionMask::South));

            glyphs.push_back(Glyph {PlaceType::Wall, State::None, mask, false,
                across && along ? corner : across ? horizontal : along ? vertical : opening});
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
    // A cell per glyph has nowhere to put an edge, so the reduction feeding this emits no
    // wall at all and a door only where one occupies a cell of its own
    // An actor line first, so that it wins the tie against a line asking for one state flag:
    // an actor in an open doorway draws as the actor rather than as the doorway
    static const Glyphs glyphs(std::vector<Glyph>
    {
        {PlaceType::Floor, State::None, {}, true, '@'},
        {PlaceType::Door, State::None, {}, true, '@'},
        {PlaceType::Solid, State::None, {}, false, '#'},
        {PlaceType::Floor, State::None, {}, false, '.'},
        {PlaceType::Door, State::None, {}, false, '/'},
        {PlaceType::Door, State::Open, {}, false, '\\'}
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
            {PlaceType::Floor, State::None, {}, true, '@'},
            {PlaceType::Door, State::None, {}, true, '@'},
            {PlaceType::Solid, State::None, {}, false, '#'},
            {PlaceType::None, State::None, {}, false, ' '},
            {PlaceType::Floor, State::None, {}, false, ' '},
            {PlaceType::Door, State::None, {}, false, '/'},
            {PlaceType::Door, State::Open, {}, false, '\\'}
        };

        add_walls(v, '-', '|', '+', ' ');
        return Glyphs(std::move(v));
    }();

    return glyphs;
}

//------------------------------------------------------------------------------
quetzal::stratum_view::Console quetzal::stratum_view::render(const Cells& cells, const Glyphs& glyphs)
{
    const vector_type& extent = cells.extent();
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
quetzal::stratum_view::Console quetzal::stratum_view::render(const sim::World& world, flecs::entity zone, const Glyphs& glyphs)
{
    return render(cells(world, zone), glyphs);
}

//------------------------------------------------------------------------------
quetzal::stratum_view::Console quetzal::stratum_view::render_lattice(const Lattice& lattice, const Glyphs& glyphs)
{
    const vector_type& extent = lattice.extent();
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
quetzal::stratum_view::Console quetzal::stratum_view::render_lattice(const sim::World& world, flecs::entity zone, const Glyphs& glyphs)
{
    return render_lattice(lattice(world, zone), glyphs);
}
