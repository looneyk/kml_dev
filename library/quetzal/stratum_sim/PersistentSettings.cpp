//------------------------------------------------------------------------------
// stratum_sim
// PersistentSettings.cpp
//------------------------------------------------------------------------------

#include "PersistentSettings.hpp"
#include "quetzal/settings/SettingsSnapshot.hpp"
#include "quetzal/stratum_format/SettingsDocument.hpp"
#include <algorithm>
#include <format>
#include <stdexcept>
#include <string>
#include <vector>

//------------------------------------------------------------------------------
quetzal::stratum_sim::PersistentSettings::PersistentSettings(const std::filesystem::path& filepath, settings::SettingScope scope,
    settings::Settings& store, settings::SettingGroups& group_store) :
    m_filepath(filepath),
    m_scope(scope),
    m_settings(store),
    m_setting_groups(group_store),
    m_change_listener_id(store.add_change_listener([this](const std::vector<std::string>& keys_changed)
    {
        m_changed = m_changed || std::ranges::any_of(keys_changed, [this](const std::string& key) { return m_settings.setting(key).scope == m_scope; });
    })),
    m_selection_listener_id(group_store.add_selection_listener([this](const std::vector<std::string>& group_keys)
    {
        m_changed = m_changed || std::ranges::any_of(group_keys, [this](const std::string& key) { return m_setting_groups.group(key).scope() == m_scope; });
    }))
{
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::PersistentSettings::~PersistentSettings()
{
    m_settings.remove_listener(m_change_listener_id);
    m_setting_groups.remove_listener(m_selection_listener_id);
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::PersistentSettings::load()
{
    if (std::filesystem::exists(m_filepath))
    {
        stratum_format::SettingsDocument document = stratum_format::read_settings(m_filepath);

        try
        {
            document.snapshot.restore(m_scope, m_settings, m_setting_groups);
        }
        catch (const std::runtime_error& e)
        {
            throw std::runtime_error(std::format("{}: {}", m_filepath.string(), e.what()));
        }
    }

    m_changed = false;
    return;
}

//------------------------------------------------------------------------------
bool quetzal::stratum_sim::PersistentSettings::changed() const
{
    return m_changed;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::PersistentSettings::save()
{
    stratum_format::SettingsDocument document;
    document.version = std::string(stratum_format::SettingsDocument::format_version);
    document.snapshot = settings::SettingsSnapshot::capture(m_scope, m_settings, m_setting_groups);

    std::filesystem::create_directories(m_filepath.parent_path());
    stratum_format::write(m_filepath, document);
    m_changed = false;
    return;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::PersistentSettings::save_if_changed()
{
    if (m_changed)
    {
        save();
    }

    return;
}
