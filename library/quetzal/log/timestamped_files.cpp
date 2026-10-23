//------------------------------------------------------------------------------
// log
// timestamped_files.cpp
//------------------------------------------------------------------------------

#include "timestamped_files.hpp"
#include <algorithm>
#include <cctype>
#include <format>
#include <string>
#include <string_view>

namespace
{

    // The form of the time in a timestamped file's name, in which each '#' matches a digit
    constexpr std::string_view time_form = "########T######.###Z";

    //--------------------------------------------------------------------------
    // Returns true if the filename is the base path's stem and '_', a time, and the base path's extension
    bool is_timestamped_filename(std::string_view filename, std::string_view prefix, std::string_view extension)
    {
        if (filename.size() != prefix.size() + time_form.size() + extension.size() || !filename.starts_with(prefix)
            || !filename.ends_with(extension))
        {
            return false;
        }

        const std::string_view time = filename.substr(prefix.size(), time_form.size());
        return std::ranges::equal(time, time_form, [](char c, char form_c)
        {
            return form_c == '#' ? std::isdigit(static_cast<unsigned char>(c)) != 0 : c == form_c;
        });
    }

} // namespace

//------------------------------------------------------------------------------
std::filesystem::path quetzal::log::timestamped_filepath(const std::filesystem::path& base_filepath, std::chrono::system_clock::time_point time)
{
    const auto time_milliseconds = std::chrono::floor<std::chrono::milliseconds>(time);
    std::filesystem::path filepath = base_filepath;
    filepath.replace_filename(std::format("{}_{:%Y%m%dT%H%M%S}Z{}", base_filepath.stem().string(), time_milliseconds,
        base_filepath.extension().string()));
    return filepath;
}

//------------------------------------------------------------------------------
std::vector<quetzal::log::UnremovedFile> quetzal::log::remove_oldest_timestamped_files(const std::filesystem::path& base_filepath,
    std::size_t file_count_max)
{
    std::vector<UnremovedFile> unremoved_files;

    if (file_count_max == 0)
    {
        return unremoved_files;
    }

    const std::filesystem::path directory = base_filepath.has_parent_path() ? base_filepath.parent_path() : std::filesystem::path(".");
    if (!std::filesystem::is_directory(directory))
    {
        return unremoved_files;
    }

    const std::string prefix = base_filepath.stem().string() + "_";
    const std::string extension = base_filepath.extension().string();
    std::vector<std::filesystem::path> filepaths;

    for (const auto& entry : std::filesystem::directory_iterator(directory))
    {
        if (entry.is_regular_file() && is_timestamped_filename(entry.path().filename().string(), prefix, extension))
        {
            filepaths.push_back(entry.path());
        }
    }

    if (filepaths.size() <= file_count_max)
    {
        return unremoved_files;
    }

    std::ranges::sort(filepaths);

    for (std::size_t i = 0; i < filepaths.size() - file_count_max; ++i)
    {
        std::error_code error;
        if (!std::filesystem::remove(filepaths[i], error))
        {
            unremoved_files.push_back(UnremovedFile {filepaths[i], error});
        }
    }

    return unremoved_files;
}
