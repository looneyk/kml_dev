#if !defined(QUETZAL_STRATUM_FORMAT_SETTINGSDOCUMENT_HPP)
#define QUETZAL_STRATUM_FORMAT_SETTINGSDOCUMENT_HPP
//------------------------------------------------------------------------------
// stratum_format
// SettingsDocument.hpp
//------------------------------------------------------------------------------

#include "types.hpp"
#include "quetzal/settings/SettingsSnapshot.hpp"
#include <filesystem>
#include <iosfwd>
#include <string>
#include <string_view>
#include <vector>

namespace quetzal::stratum_format
{

    //--------------------------------------------------------------------------
    // The data structure analog of a settings file, such as the user settings file: one scope's snapshot
    struct SettingsDocument
    {
        static constexpr std::string_view format_name = "quetzal.stratum_settings";
        static constexpr std::string_view format_version = "0.1";

        std::string version;
        settings::SettingsSnapshot snapshot;
    };

    // Throws std::runtime_error on error or an unsupported key, stream_name used in error messages
    SettingsDocument read_settings(const std::filesystem::path& filepath);
    SettingsDocument read_settings(std::istream& is, const std::string& stream_name);

    void write(const std::filesystem::path& filepath, const SettingsDocument& document);
    void write(std::ostream& os, const SettingsDocument& document);

    namespace detail
    {

        // Reads a file's settings and groups, either of which may be absent
        settings::SettingsSnapshot read_snapshot(const raw_values_type& v);

        // Adds the snapshot's settings and groups, each a section on one line
        void add_snapshot_sections(std::vector<std::string>& sections, const settings::SettingsSnapshot& snapshot);

    } // namespace detail

} // namespace quetzal::stratum_format

#endif // QUETZAL_STRATUM_FORMAT_SETTINGSDOCUMENT_HPP
