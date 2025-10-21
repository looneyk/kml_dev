#if !defined(QUETZAL_MODEL_PATTERN_PAGE_HPP)
#define QUETZAL_MODEL_PATTERN_PAGE_HPP
//------------------------------------------------------------------------------
// model_pattern
// Page.hpp
//------------------------------------------------------------------------------

#include "Pane.hpp"
#include "quetzal/geometry/Point.hpp"
#include "quetzal/geometry/Box.hpp"
#include "quetzal/math/HomogeneousTraits.hpp"
#include "quetzal/pdf/Document.hpp"
#include <sstream>
#include <string>
#include <vector>

namespace quetzal
{

    //--------------------------------------------------------------------------
    class Page
    {
    public:

        using traits_type = math::HomogeneousTraits<double, 2>;
        using point_type = geometry::Point<traits_type>;
        using vector_type = point_type::vector_type;
        using box_type = geometry::Box<traits_type>;

        Page(const vector_type& size, double margin, double separation, const std::string& font, double font_size);
        Page(const Page&) = default;
        Page(Page&&) = default;
        ~Page() = default;

        Page& operator=(const Page&) = default;
        Page& operator=(Page&&) = default;

        bool insert(const Pane& pane);

        void draw(pdf::Document& document) const;

    private:

        //----------------------------------------------------------------------
        struct Placement
        {
            Placement(const Pane& pane, const point_type& position) :
                m_pane(pane),
                m_position(position)
            {
            }

            Placement(const Placement&) = default;
            Placement(Placement&&) = default;
            ~Placement() = default;

            Placement& operator=(const Placement&) = default;
            Placement& operator=(Placement&&) = default;

            Pane m_pane;
            point_type m_position;
        };

        vector_type m_size;
        double m_margin;
        double m_separation;

        std::string m_font;
        double m_font_size;

        std::vector<Placement> m_placements;

        // layout management ...
        std::vector<box_type> m_free;
    };

} // namespace quetzal

#endif // QUETZAL_MODEL_PATTERN_PAGE_HPP
