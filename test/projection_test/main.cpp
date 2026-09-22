//------------------------------------------------------------------------------
// projection_test
// main.cpp
//
// Exercises geometry::Projection itself. The maps built on top of it are covered by mapping_test.
//------------------------------------------------------------------------------

#include "quetzal/geometry/Point.hpp"
#include "quetzal/geometry/Projection.hpp"
#include "quetzal/math/HomogeneousTraits.hpp"
#include "quetzal/math/floating_point.hpp"
#include <format>
#include <iostream>
#include <string>

using namespace std;
using namespace quetzal;

namespace
{

    using value_type = double;
    using vector_traits = math::HomogeneousTraits<value_type, 3>;
    using point_type = geometry::Point<vector_traits>;
    using vector_type = point_type::vector_type;
    using projection_type = geometry::Projection<vector_traits>;
    using projected_type = projection_type::projected_type;

    size_t n_cases = 0;
    size_t n_failures = 0;

    //--------------------------------------------------------------------------
    void check(const string& name, bool b, const string& detail = {})
    {
        ++n_cases;
        if (!b)
        {
            ++n_failures;
        }

        cout << (b ? "pass  " : "FAIL  ") << name;
        if (!detail.empty())
        {
            cout << "  " << detail;
        }

        cout << endl;
        return;
    }

    //--------------------------------------------------------------------------
    bool near(const projected_type& a, const projected_type& b, value_type tolerance = 1.0e-9)
    {
        return abs(a.x() - b.x()) <= tolerance && abs(a.y() - b.y()) <= tolerance;
    }

    //--------------------------------------------------------------------------
    string to_string(const projected_type& t)
    {
        return std::format("({:.9f}, {:.9f})", t.x(), t.y());
    }

    //--------------------------------------------------------------------------
    // Projection itself, independent of any mapping
    void test_projection()
    {
        cout << endl << "projection" << endl;

        projection_type projection_z(vector_type(0.0, 0.0, 1.0));
        check("projection z: origin", near(projection_z(point_type(0.0, 0.0, 0.0)), projected_type(0.0, 0.0)),
            std::format("got {}", to_string(projection_z(point_type(0.0, 0.0, 0.0)))));

        // Points differing only in the projection direction must project to the same point
        projected_type a = projection_z(point_type(1.0, 2.0, 0.0));
        projected_type b = projection_z(point_type(1.0, 2.0, 5.0));
        check("projection z: depth is discarded", near(a, b), std::format("{} vs {}", to_string(a), to_string(b)));

        // Distances within the plane are preserved by an orthographic projection of that plane
        projection_type projection_x(vector_type(1.0, 0.0, 0.0));
        projected_type p0 = projection_x(point_type(0.0, 0.0, 0.0));
        projected_type p1 = projection_x(point_type(0.0, 3.0, 4.0));
        value_type d = sqrt((p1.x() - p0.x()) * (p1.x() - p0.x()) + (p1.y() - p0.y()) * (p1.y() - p0.y()));
        check("projection x: in-plane distance preserved", abs(d - 5.0) <= 1.0e-9, std::format("got {:.9f} expected 5", d));
        return;
    }

} // namespace

//------------------------------------------------------------------------------
int main()
{
    test_projection();

    cout << endl << (n_failures == 0 ? "OK: " : "Error: ") << (n_cases - n_failures) << " of " << n_cases << " cases passed" << endl;
    return n_failures == 0 ? 0 : 1;
}
