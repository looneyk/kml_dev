#if !defined(QUETZAL_MODEL_PATTERN_PANES_HPP)
#define QUETZAL_MODEL_PATTERN_PANES_HPP
//------------------------------------------------------------------------------
// model_pattern
// Panes.hpp
//------------------------------------------------------------------------------

#include "Pane.hpp"
#include <vector>

namespace quetzal
{

    //--------------------------------------------------------------------------
    class Panes
    {
    public:

        using traits_type = math::HomogeneousTraits<double, 2>;
        using point_type = geometry::Point<traits_type>;
        using vector_type = point_type::vector_type;
        using polygon_type = geometry::Polygon<traits_type>;
        using panes_type = std::vector<Pane>;

        Panes() = default;
        Panes(const Panes&) = default;
        Panes(Panes&&) = default;
        ~Panes() = default;

        Panes& operator=(const Panes&) = default;
        Panes& operator=(Panes&&) = default;

        panes_type::const_iterator begin() const;
        panes_type::iterator begin();
        panes_type::const_iterator end() const;
        panes_type::iterator end();

        // Panes is just a vector/list/... of Pane with the following auxiliary functions? ...

        size_t instance_count() const; // Unique panes
        size_t reference_count() const; // Total panes
        double area() const; // Total rectangular area of Pane bounds

        panes_type::iterator find(const Pane& pane);
        void insert(const Pane& pane);

        void sort();

    private:

        panes_type m_panes;
    };

} // namespace quetzal

#endif // QUETZAL_MODEL_PATTERN_PANES_HPP
