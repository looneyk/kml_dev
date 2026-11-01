#if !defined(QUETZAL_TRIANGULATION_TRIANGULATION_HPP)
#define QUETZAL_TRIANGULATION_TRIANGULATION_HPP
//------------------------------------------------------------------------------
// triangulation
// triangulation.hpp
//------------------------------------------------------------------------------

#include <array>
#include <cstddef>
#include <numbers>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace quetzal::triangulation
{

    //--------------------------------------------------------------------------
    struct Point
    {
        double x;
        double y;
    };

    using edge_type = std::array<size_t, 2>;
    using triangle_type = std::array<size_t, 3>;

    //--------------------------------------------------------------------------
    // A point's slot counts through each boundary in turn and then the interior points, and the points a triangulation adds
    // take the slots after those. Edges and triangles refer to points by slot. The region is the area enclosed by an odd number of boundaries, so a boundary inside
    // another is a hole whatever its winding.
    struct Region
    {
        std::vector<std::vector<Point>> boundaries;
        std::vector<Point> interior_points;
    };

    //--------------------------------------------------------------------------
    struct Refinement
    {
        enum class Criterion
        {
            smallest_angle,
            largest_area
        };

        size_t added_point_limit = 0;
        Criterion criterion = Criterion::smallest_angle;
        double threshold = std::numbers::pi / 9.0; // The smallest angle in radians, or the largest area
        double edge_length_minimum = 1e-6; // An edge or triangle this small is not split
    };

    //--------------------------------------------------------------------------
    // Each condition is an Error unless its option is set
    struct Options
    {
        bool duplicate_points_merged = false;
        bool crossing_edges_split = false;
        bool edges_split_at_points = false;
        bool overlapping_edges_permitted = false;
        bool conforming = false;
        double point_on_edge_tolerance = 0.0; // A fraction of the edge's length
        std::optional<Refinement> refinement;
    };

    //--------------------------------------------------------------------------
    struct AddedPoint
    {
        Point position;
        std::vector<edge_type> edges; // The input edges the point divides, which are none for a point inside the region
    };

    //--------------------------------------------------------------------------
    struct Triangulation
    {
        std::vector<triangle_type> triangles; // Counterclockwise
        std::vector<AddedPoint> added_points;
        std::vector<std::array<size_t, 2>> merged_point_slots; // <merged point's slot, retained point's slot>
    };

    //--------------------------------------------------------------------------
    class Error : public std::runtime_error
    {
    public:

        enum class Condition
        {
            duplicate_points,
            crossing_edges,
            unplaceable_intersection,
            point_on_edge,
            overlapping_edges
        };

        Error(Condition condition, const std::vector<size_t>& point_slots, const std::string& message);

        Condition condition() const;

        // The slots of the points involved, in the order the message lists them
        const std::vector<size_t>& point_slots() const;

    private:

        Condition m_condition;
        std::vector<size_t> m_point_slots;
    };

    //--------------------------------------------------------------------------
    // Constrained Delaunay triangulation of the region
    Triangulation triangulate(const Region& region, const Options& options = {});

} // namespace quetzal::triangulation

#endif // QUETZAL_TRIANGULATION_TRIANGULATION_HPP
