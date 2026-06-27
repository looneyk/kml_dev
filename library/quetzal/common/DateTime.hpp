#if !defined(QUETZAL_DATETIME_HPP)
#define QUETZAL_DATETIME_HPP
//------------------------------------------------------------------------------
// DateTime.hpp
//------------------------------------------------------------------------------

#include <string>
#include <string_view>
#include <chrono>
#include <iosfwd>

namespace quetzal
{

    //--------------------------------------------------------------------------
    class DateTime
    {
    public:

        DateTime();
        DateTime(const DateTime&) = default;
        DateTime(DateTime&&) = default;
        ~DateTime();

        DateTime& operator=(const DateTime&) = default;
        DateTime& operator=(DateTime&&) = default;

        // Default format is ISO 8601
        std::string to_string(std::string_view spec = "") const;
        bool from_string(const std::string_view str, std::string_view spec = "");

        std::string to_string_utc(std::string_view spec = "") const;
        bool from_string_utc(const std::string_view str, std::string_view spec = "");

        int days_until(const DateTime& dt) const; // Assumes that dt is later

    private:

        std::chrono::system_clock::time_point m_tp;
    };

    std::ostream& operator<<(std::ostream& os, const DateTime& dt);
    std::istream& operator>>(std::istream& is, DateTime& dt);
	
} // namespace quetzal

#endif // QUETZAL_DATETIME_HPP
