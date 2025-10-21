//------------------------------------------------------------------------------
// vector_point
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/math/Point.hpp"
#include "quetzal/math/Vector.hpp"
#include "quetzal/math/VectorTraits.hpp"
#include <cassert>

using namespace std;
using namespace quetzal;

namespace
{

    using TraitsF3 = math::VectorTraits<float, 3>;
    using VectorF3 = math::Vector<TraitsF3>;
    using PointF3 = math::Point<TraitsF3>;

} // namespace

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    argc;
    argv;

    VectorF3 v1(1.0f, 2.0f, 3.0f);
    VectorF3 v2(2.0f, 3.0f, 4.0f);

    PointF3 p1(1.0f, 2.0f, 3.0f);
    PointF3 p2(2.0f, 3.0f, 4.0f);

    VectorF3 v = p2 - p2;
    PointF3 p3 = p1 + v1;
    PointF3 p4 = v1 + p1;
    PointF3 p5 = p1 * 2.0f;
    PointF3 p6 = 2.0f * p1;

//    auto r = p1 + p2;
//    VectorF3 vv = p1;
//    PointF3 pp = v1;

    return 0;
}
