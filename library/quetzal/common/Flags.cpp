//------------------------------------------------------------------------------
// common
// Flags.cpp
//------------------------------------------------------------------------------

#include "Flags.hpp"
#include <type_traits>
#include <iostream>
#include <string>
#include <cassert>

using namespace std;

namespace
{

    //--------------------------------------------------------------------------
    struct Bits
    {
        static constexpr uint32_t Deleted     = 0x00000001;
        static constexpr uint32_t Visited     = 0x00000002;
        static constexpr uint32_t Checked     = 0x00000004;
        static constexpr uint32_t Marked      = 0x00000008;
        static constexpr uint32_t Comparison  = 0x00000030;
    };

    static constexpr uint32_t ComparisonShift = 4;

    // Comaprison bit field three-way comparison results
    // 00  Uncompared
    // 01  -1
    // 10   0
    // 11   1

} // namespace

//------------------------------------------------------------------------------
bool quetzal::Flags::deleted() const
{
    return (m_bits & Bits::Deleted) != 0;
}

//------------------------------------------------------------------------------
void quetzal::Flags::set_deleted(bool b) const
{
    if (b)
    {
        m_bits |= Bits::Deleted;
    }
    else
    {
        m_bits &= ~Bits::Deleted;
    }

    return;
}

//------------------------------------------------------------------------------
bool quetzal::Flags::visited() const
{
    return (m_bits & Bits::Visited) != 0;
}

//------------------------------------------------------------------------------
void quetzal::Flags::set_visited(bool b) const
{
    if (b)
    {
        m_bits |= Bits::Visited;
    }
    else
    {
        m_bits &= ~Bits::Visited;
    }

    return;
}

//------------------------------------------------------------------------------
bool quetzal::Flags::checked() const
{
    return (m_bits & Bits::Checked) != 0;
}

//------------------------------------------------------------------------------
void quetzal::Flags::set_checked(bool b) const
{
    if (b)
    {
        m_bits |= Bits::Checked;
    }
    else
    {
        m_bits &= ~Bits::Checked;
    }

    return;
}

//------------------------------------------------------------------------------
bool quetzal::Flags::marked() const
{
    return (m_bits & Bits::Marked) != 0;
}

//------------------------------------------------------------------------------
void quetzal::Flags::set_marked(bool b) const
{
    if (b)
    {
        m_bits |= Bits::Marked;
    }
    else
    {
        m_bits &= ~Bits::Marked;
    }

    return;
}

//------------------------------------------------------------------------------
bool quetzal::Flags::compared() const
{
    return (m_bits & Bits::Comparison) != 0;
}

//------------------------------------------------------------------------------
int quetzal::Flags::comparison() const
{
    assert(compared());
    return ((m_bits & Bits::Comparison) >> ComparisonShift) - 2;
}

//------------------------------------------------------------------------------
void quetzal::Flags::set_comparison(int value) const
{
    assert(value >= -2 && value <= 1);
    m_bits = m_bits & ~Bits::Comparison | ((value + 2) << ComparisonShift) & Bits::Comparison;
    return;
}

//------------------------------------------------------------------------------
bool quetzal::Flags::empty() const
{
    return m_bits == 0;
}

//------------------------------------------------------------------------------
void quetzal::Flags::clear() const
{
    m_bits = 0;
    return;
}

//------------------------------------------------------------------------------
void quetzal::Flags::clear_all() const
{
    clear();
    return;
}

//------------------------------------------------------------------------------
bool quetzal::Flags::unset() const
{
    return deleted() || empty();
}

//------------------------------------------------------------------------------
void quetzal::Flags::reset() const
{
    m_bits &= Bits::Deleted;
    return;
}

//------------------------------------------------------------------------------
void quetzal::Flags::reset_all() const
{
    reset();
    return;
}

//------------------------------------------------------------------------------
ostream& quetzal::operator<<(ostream& os, const Flags& flags)
{
    os << (flags.deleted() ? 'D' : '-');
    os << (flags.visited() ? 'V' : '-');
    os << (flags.checked() ? 'C' : '-');
    os << (flags.marked() ? 'M' : '-');
    os << '<' << (flags.compared() ? to_string(flags.comparison()) : "-") << '>';
    return os;
}
