//------------------------------------------------------------------------------
// polygonal_approximation_test
//------------------------------------------------------------------------------

#include "quetzal/math/PolygonalApproximation.hpp"
#include "quetzal/math/Vector2.hpp"

using namespace std;

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    typedef Vector2<float> point_type;

    Curve<point_type> curve(...);

    PolygonalApproximation<point_type> approximation(curve);

    vertices ...
}
