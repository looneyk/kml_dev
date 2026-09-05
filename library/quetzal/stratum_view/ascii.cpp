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
    using quetzal::stratum_view::Kind;
    using quetzal::stratum_view::Mask;
    using quetzal::stratum_view::Place;
    using quetzal::stratum_view::State;
    using quetzal::stratum_view::any;

    //--------------------------------------------------------------------------
    // How much of a place a line names, a state flag and a mask counting one each. A line
    // naming more of it is preferred to one naming less
    int specificity(const Glyph& glyph)
    {
        int n = glyph.mask.has_value() ? 1 : 0;

        for (uint16_t bit = 1; bit != 0; bit = static_cast<uint16_t>(bit << 1))
        {
            n += any(glyph.state & static_cast<State>(bit)) ? 1 : 0;
        }

        return n;
    }

    //--------------------------------------------------------------------------
    bool matches(const Glyph& glyph, const Place& place)
    {
        return glyph.kind == place.kind
            && (glyph.state & place.state) == glyph.state
            && (!glyph.mask.has_value() || *glyph.mask == place.mask);
    }

    //--------------------------------------------------------------------------
    // ASCII has three glyphs for the sixteen joints the mask tells apart, so a wall is
    // sixteen lines: a run passing through horizontally or vertically, a junction where both
    // axes reach it, and nothing where neither does
    void add_walls(std::vector<Glyph>& glyphs, char horizontal, char vertical, char corner, char opening)
    {
        for (uint8_t bits = 0; bits < 16; ++bits)
        {
            Mask mask = static_cast<Mask>(bits);

            bool across = any(mask & (Mask::East | Mask::West));
            bool along = any(mask & (Mask::North | Mask::South));

            glyphs.push_back(Glyph {Kind::Wall, State::None, mask,
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
    // wall at all and a door only where one stands in a cell of its own
    static const Glyphs glyphs(std::vector<Glyph>
    {
        {Kind::Rock, State::None, {}, '#'},
        {Kind::Floor, State::None, {}, '.'},
        {Kind::Actor, State::None, {}, '@'},
        {Kind::Door, State::None, {}, '/'},
        {Kind::Door, State::Open, {}, '\\'}
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
            {Kind::Rock, State::Solid, {}, '#'},
            {Kind::Rock, State::None, {}, ' '},
            {Kind::Floor, State::None, {}, ' '},
            {Kind::Actor, State::None, {}, '@'},
            {Kind::Door, State::None, {}, '/'},
            {Kind::Door, State::Open, {}, '\\'}
        };

        add_walls(v, '-', '|', '+', ' ');
        return Glyphs(std::move(v));
    }();

    return glyphs;
}

//------------------------------------------------------------------------------
quetzal::stratum_view::Console quetzal::stratum_view::render(const Cells& cells, const Glyphs& glyphs)
{
    Console console(cells.width(), cells.height(), ' ');

    for (size_t j = 0; j < cells.height(); ++j)
    {
        for (size_t i = 0; i < cells.width(); ++i)
        {
            console.set(i, j, glyphs(cells.at(i, j)));
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
    Console console(lattice.width(), lattice.height(), ' ');

    for (size_t j = 0; j < lattice.height(); ++j)
    {
        for (size_t i = 0; i < lattice.width(); ++i)
        {
            console.set(i, j, glyphs(lattice.at(i, j)));
        }
    }

    return console;
}

//------------------------------------------------------------------------------
quetzal::stratum_view::Console quetzal::stratum_view::render_lattice(const sim::World& world, flecs::entity zone, const Glyphs& glyphs)
{
    return render_lattice(lattice(world, zone), glyphs);
}
