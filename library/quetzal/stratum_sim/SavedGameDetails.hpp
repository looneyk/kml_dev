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
    class SavedGameDetails
    {
    public:

        using time_type = std::filesystem::file_time_type;

        SavedGameDetails(std::string_view name, std::string_view description, const time_type& time);
        SavedGameDetails(const SavedGameDetails&) = default;
        SavedGameDetails(SavedGameDetails&&) = default;
        ~SavedGameDetails() = default;

        SavedGameDetails& operator=(const SavedGameDetails&) = default;
        SavedGameDetails& operator=(SavedGameDetails&&) = default;

    private:

        std::string m_name;
        std::string m_description;
        time_type m_time;
        // game type ...
        // preview, thumbnail ...
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_SAVEDGAMEDETAILS_HPP
