#if !defined(QUETZAL_STRATUM_SIM_PERSISTENTSETTINGS_HPP)
#define QUETZAL_STRATUM_SIM_PERSISTENTSETTINGS_HPP
//------------------------------------------------------------------------------
// stratum_sim
// PersistentSettings.hpp
//------------------------------------------------------------------------------

#include "quetzal/settings/Setting.hpp"
#include "quetzal/settings/Settings.hpp"
#include <filesystem>
#include <string>
#include <string_view>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // One scope's settings, kept in a settings file. Tracks whether they changed since they were loaded or saved, so the
    // file is written only when the program closes and only if they did
    class PersistentSettings
    {
    public:

        // Adds a change listener to the store, which must outlive it, and removes it when destroyed
        PersistentSettings(const std::filesystem::path& filepath, std::string_view scope, settings::Settings& store);
        PersistentSettings(const PersistentSettings&) = delete;
        PersistentSettings(PersistentSettings&&) = delete;
        ~PersistentSettings();

        PersistentSettings& operator=(const PersistentSettings&) = delete;
        PersistentSettings& operator=(PersistentSettings&&) = delete;

        // Restores the file's snapshot, if there is a file, and counts none of it as a change. Throws std::runtime_error,
        // naming the file, if it cannot be read or restored
        void load();

        // Returns true if a setting of the scope changed since the settings were loaded or saved
        bool changed() const;

        // Writes the scope's snapshot, creating the file's directory if needed
        void save();
        void save_if_changed();

    private:

        std::filesystem::path m_filepath;
        std::string m_scope;
        settings::Settings& m_settings;
        settings::Settings::listener_id_type m_change_listener_id;
        bool m_changed = false;
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_PERSISTENTSETTINGS_HPP
