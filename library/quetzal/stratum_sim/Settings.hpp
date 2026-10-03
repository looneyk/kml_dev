#if !defined(QUETZAL_STRATUM_SIM_SETTINGS_HPP)
#define QUETZAL_STRATUM_SIM_SETTINGS_HPP
//------------------------------------------------------------------------------
// stratum_sim
// Settings.hpp
//------------------------------------------------------------------------------

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // How opening a save sets the state of the game's random engine, which is the same engine either way: restored to the
    // state saved, so the game continues with the same outcomes however often the save is opened, or reseeded, so each
    // opening has outcomes of its own
    enum class RandomSeedingOnOpen
    {
        RestoreSavedState,
        Reseed
    };

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

        RandomSeedingOnOpen random_seeding_on_open() const;
        void set_random_seeding_on_open(RandomSeedingOnOpen random_seeding_on_open);

    private:

        // ...

        RandomSeedingOnOpen m_random_seeding_on_open;
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_SETTINGS_HPP
