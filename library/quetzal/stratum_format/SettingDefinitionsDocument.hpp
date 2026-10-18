#if !defined(QUETZAL_STRATUM_FORMAT_SETTINGDEFINITIONSDOCUMENT_HPP)
#define QUETZAL_STRATUM_FORMAT_SETTINGDEFINITIONSDOCUMENT_HPP
//------------------------------------------------------------------------------
// stratum_format
// SettingDefinitionsDocument.hpp
//------------------------------------------------------------------------------

#include "quetzal/common/KeyValueStore.hpp"
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace quetzal::stratum_format
{

    //--------------------------------------------------------------------------
    struct SettingDefinitionsDocument
    {
        static constexpr std::string_view format_name = "quetzal.stratum_setting_definitions";
        static constexpr std::string_view format_version = "0.1";

        // Relative to the assets or a mod directory
        static constexpr std::string_view filepath = "data/setting_definitions.json";

        enum class Scope
        {
            User,
            Game
        };

        struct Enumeration
        {
            std::string name;
            std::vector<std::string> values;
        };

        struct Setting
        {
            std::string key;
            std::string type_name;
            std::string default_value;
            Scope scope = Scope::User;
            bool restart_required = false;
            std::string display_name;
            std::string description;
            std::string category;
            std::optional<std::string> minimum;
            std::optional<std::string> maximum;
        };

        struct Preset
        {
            std::string name;
            std::string display_name;
            bool editable = false;
            KeyValueStore values;
        };

        struct Group
        {
            std::string key;
            Scope scope = Scope::User;
            std::string default_preset_name;
            std::string display_name;
            std::string category;
            std::vector<Preset> presets;
        };

        std::string version;
        std::vector<Enumeration> enumerations;
        std::vector<Setting> settings;
        std::vector<Group> groups;
    };

} // namespace quetzal::stratum_format

#endif // QUETZAL_STRATUM_FORMAT_SETTINGDEFINITIONSDOCUMENT_HPP
