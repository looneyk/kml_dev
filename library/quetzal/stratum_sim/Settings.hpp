#if !defined(QUETZAL_STRATUM_SIM_SETTINGS_HPP)
#define QUETZAL_STRATUM_SIM_SETTINGS_HPP
//------------------------------------------------------------------------------
// stratum_sim
// Settings.hpp
//------------------------------------------------------------------------------

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    class Settings
    {
    public:

        Settings();
        Settings(const Settings&) = default;
        Settings(Settings&&) = default;
        ~Settings() = default;

        Settings& operator=(const Settings&) = default;
        Settings& operator=(Settings&&) = default;

    private:

        // ...

        int m_placeholder;
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_SETTINGS_HPP
