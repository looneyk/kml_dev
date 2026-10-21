#if !defined(QUETZAL_STRATUM_SIM_SAVEDGAMEDETAILS_HPP)
#define QUETZAL_STRATUM_SIM_SAVEDGAMEDETAILS_HPP
//------------------------------------------------------------------------------
// stratum_sim
// SavedGameDetails.hpp
//------------------------------------------------------------------------------

#include <chrono>
#include <filesystem>
#include <string>
#include <string_view>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // The type of a save: an explicit save, an autosave, or a quick save. They differ only in the save's name
    enum class SaveType
    {
        Explicit,
        Auto,
        Quick
    };

    //--------------------------------------------------------------------------
    // A save as a menu lists it. The save name is its directory's, which Game::open takes, and the character's name is as
    // the player entered it
    class SavedGameDetails
    {
    public:

        using time_type = std::filesystem::file_time_type;

        SavedGameDetails(std::string_view save_name, std::string_view character_name, std::string_view description, const time_type& time,
            SaveType save_type);
        SavedGameDetails(const SavedGameDetails&) = default;
        SavedGameDetails(SavedGameDetails&&) = default;
        ~SavedGameDetails() = default;

        SavedGameDetails& operator=(const SavedGameDetails&) = default;
        SavedGameDetails& operator=(SavedGameDetails&&) = default;

        const std::string& save_name() const;
        const std::string& character_name() const;
        const std::string& description() const;
        const time_type& time() const;
        SaveType save_type() const;

    private:

        std::string m_save_name;
        std::string m_character_name;
        std::string m_description;
        time_type m_time;
        SaveType m_save_type;
        // game type ...
        // preview, thumbnail ...
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_SAVEDGAMEDETAILS_HPP
