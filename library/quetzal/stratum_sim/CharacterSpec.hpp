#if !defined(QUETZAL_STRATUM_SIM_CHARACTERSPEC_HPP)
#define QUETZAL_STRATUM_SIM_CHARACTERSPEC_HPP
//------------------------------------------------------------------------------
// stratum_sim
// CharacterSpec.hpp
//------------------------------------------------------------------------------

#include <vector>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    class CharacterSpec
    {
    public:

        CharacterSpec();
        CharacterSpec(const CharacterSpec&) = default;
        CharacterSpec(CharacterSpec&&) = default;
        ~CharacterSpec() = default;

        CharacterSpec& operator=(const CharacterSpec&) = default;
        CharacterSpec& operator=(CharacterSpec&&) = default;

    private:

        // class, race, history, appearance
        // stats
        // skills
        // spells
        // inventory
        // level 1
        int m_placeholder;
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_CHARACTERSPEC_HPP
