#if !defined(QUETZAL_STRATUM_SIM_REGIONSPEC_HPP)
#define QUETZAL_STRATUM_SIM_REGIONSPEC_HPP
//------------------------------------------------------------------------------
// stratum_sim
// RegionSpec.hpp
//------------------------------------------------------------------------------

#include <vector>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    class RegionSpec
    {
    public:

        RegionSpec();
        RegionSpec(const RegionSpec&) = default;
        RegionSpec(RegionSpec&&) = default;
        ~RegionSpec() = default;

        RegionSpec& operator=(const RegionSpec&) = default;
        RegionSpec& operator=(RegionSpec&&) = default;

    private:

        // type, terrain, competency level, algorithm, ...

        int m_placeholder;
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_REGIONSPEC_HPP
