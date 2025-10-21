#if !defined(QUETZAL_PDF_FONT_HPP)
#define QUETZAL_PDF_FONT_HPP
//------------------------------------------------------------------------------
// pdf
// Font.hpp
//------------------------------------------------------------------------------

#include "quetzal/common/id.hpp"
#include <string>
#include <vector>

namespace quetzal::pdf
{

    //--------------------------------------------------------------------------
    class Font
    {
    public:

        typedef std::vector<unsigned int> metrics_type;

        Font();
        Font(id_type id, const std::string& name, const metrics_type& metrics);
        Font(const Font&) = default;
        virtual ~Font() = default;

        Font& operator=(const Font&) = default;

        id_type id() const;
        std::string name() const;

        double width(double size, const std::string& s) const;

    private:

        id_type m_id;
        std::string m_name; // BaseFont
        metrics_type m_metrics;
    };

    void init_fonts();
    size_t font_count();
    const Font* font(id_type id);
    const Font* font(const std::string& name);

    std::vector<std::string> word_wrap(const Font& font, double size, double wMax, const std::string& s, bool bRightJustify); // Font, or separate? ...

} // namespace quetzal::pdf

#endif // QUETZAL_PDF_FONT_HPP
