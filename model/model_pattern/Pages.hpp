#if !defined(QUETZAL_MODEL_PATTERN_PAGES_HPP)
#define QUETZAL_MODEL_PATTERN_PAGES_HPP
//------------------------------------------------------------------------------
// model_pattern
// Pages.hpp
//------------------------------------------------------------------------------

#include "Page.hpp"
#include "Pane.hpp"
#include "quetzal/geometry/Point.hpp"
#include "quetzal/math/HomogeneousTraits.hpp"
#include "quetzal/pdf/Document.hpp"
#include <string>
#include <vector>

namespace quetzal
{

    //--------------------------------------------------------------------------
    class Pages
    {
    public:

        using traits_type = math::HomogeneousTraits<double, 2>;
        using point_type = geometry::Point<traits_type>;
        using vector_type = point_type::vector_type;
        using pages_type = std::vector<Page>;

        Pages(vector_type page_size, double margin, double separation, const std::string& font, double font_size);
        Pages(const Pages&) = default;
        Pages(Pages&&) = default;
        ~Pages() = default;

        Pages& operator=(const Pages&) = default;
        Pages& operator=(Pages&&) = default;

        vector_type page_client_size() const;

        pages_type::const_iterator begin() const;
        pages_type::iterator begin();
        pages_type::const_iterator end() const;
        pages_type::iterator end();

        size_t size() const;

        bool holds(const Pane& pane); // A page is large enough to contain this pane without splitting
        bool insert(const Pane& pane); // return void once split is implemented ...
        std::vector<Pane> split(const Pane& pane); // Breaks up large panes into sections each of which will fit on a page

        void draw(pdf::Document& document);

    private:

        vector_type m_page_size;
        double m_margin;
        double m_separation;
        std::string m_font;
        double m_font_size;

        pages_type m_pages;
    };

} // namespace quetzal

#endif // QUETZAL_MODEL_PATTERN_PAGES_HPP
