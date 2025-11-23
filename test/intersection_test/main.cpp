//------------------------------------------------------------------------------
// intersection_test
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/common/FileValidator.hpp"
#include "quetzal/geometry/Point.hpp"
#include "quetzal/geometry/Line.hpp"
#include "quetzal/geometry/Ray.hpp"
#include "quetzal/geometry/Segment.hpp"
#include "quetzal/geometry/Plane.hpp"
#include "quetzal/geometry/Polygon.hpp"
#include "quetzal/geometry/distance.hpp"
#include "quetzal/geometry/intersect.hpp"
#include "quetzal/math/CartesianTraits.hpp"
#include "quetzal/math/Vector.hpp"
#include "quetzal/math/HomogeneousTraits.hpp"
#include "quetzal/math/floating_point.hpp"
#include "quetzal/math/math_util.hpp"
#include <functional>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <cassert>

using namespace std;
using namespace quetzal;

namespace
{

    using value_type = float;
    using traits_type = math::HomogeneousTraits<value_type, 3>;
    using vector_type = math::Vector<traits_type>;
    using point_type = geometry::Point<traits_type>;
    using line_type = geometry::Line<traits_type>;
    using ray_type = geometry::Ray<traits_type>;
    using segment_type = geometry::Segment<traits_type>;
    using plane_type = geometry::Plane<traits_type>;
    using polygon_type = geometry::Polygon<traits_type>;

    static const filesystem::path dirOut = "out";
    static const filesystem::path dirReference = "reference";

    //--------------------------------------------------------------------------
    filesystem::path log_path()
    {
        return dirOut / "results";
    }

    //--------------------------------------------------------------------------
    template<typename... Types>
    void test(ostream& os, const string& name, function<bool()> feature, Types&&... args)
    {
        bool b = feature();
        os << (b ? "Success:\t" : "Failure:\t") << name;
        ((os << "\t[" << forward<Types>(args) << "]"), ...);
        os << endl;
        return;
    }
/*
    //--------------------------------------------------------------------------
    template<typename... Types>
    void test(ostream& os, const string& name, function<bool(const Types&...)> feature, const Types&... args)
    {
        bool b = feature(args...);
        os << (b ? "Success:\t" : "Failure:\t") << name;
        ((os << "\t[" << forward<Types>(args) << "]"), ...);
        os << endl;
        return;
    }

    //--------------------------------------------------------------------------
    template<typename T, typename U>
    void test(ostream& os, const string& name, function<bool(const T&, const U&)> feature, const T& a, const U& b)
    {
        bool b = feature(a, b);
        os << (b ? "Success:\t" : "Failure:\t") << name;
        os << "\t[" << a << "]";
        os << "\t[" << b << "]";
        os << endl;
        return;
    }
*/
    //--------------------------------------------------------------------------
    template<typename... Types>
    void value_test(ostream& os, const string& name, function<value_type()> feature, value_type result, Types&&... args)
    {
        value_type x = feature();
        os << (math::float_eq(x, result) ? "Success:\t" : "Failure:\t") << name;
        os << "\t" << x << "\t" << result;
        ((os << "\t[" << forward<Types>(args) << "]"), ...);
        os << endl;
        return;
    }

} // namespace

//------------------------------------------------------------------------------
int main(int argc, char* argv[]
{
    argc;
    argv;

    FileValidator validator(dirOut, dirReference);

    ofstream os(log_path());
    assert(os);

    point_type pointO {0.0f, 0.0f, 0.0f};
    point_type pointA {1.0f, 2.0f, 3.0f};
    point_type pointB {2.0f, 4.0f, 6.0f};
    point_type pointC {4.5f, 5.0f, 5.5f};
    point_type pointD {1.5f, 3.0f, 4.5f}; // on AB
    point_type pointE {3.0f, 3.0f, 7.0f};
    point_type pointF {1.0f, 1.0f, 7.0f};
    point_type pointN {4.0f, 5.0f, 6.0f}; // not on AB
    point_type pointP {0.0f, 1.0f, 4.0f}; // on plane
    point_type pointZ {0.0f, 0.0f, 5.0f};

    vector_type vectorAB = pointB - pointA;
    vector_type directionAB = math::normalize(vectorAB);
    vector_type vectorCD = pointD - pointC;
    vector_type directionCD = math::normalize(vectorCD);
    vector_type vectorDC = pointD - pointC;
    vector_type directionDC = math::normalize(vectorDC);
    vector_type vectorEF = pointF - pointE;
    vector_type directionEF = math::normalize(vectorEF);
    vector_type vectorDF = pointF - pointD;
    vector_type directionDF = math::normalize(vectorDF);
    vector_type vectorXY1 = {1.0f, 1.0f, 0.0f};
    vector_type directionXY1 = math::normalize(vectorXY1);
    vector_type vectorXY2 = {1.0f, -1.0f, 0.0f};
    vector_type directionXY2 = math::normalize(vectorXY2);

    line_type lineAB {pointA, directionAB};
    line_type lineCD {pointC, directionCD};
    line_type lineEF {pointE, directionEF};
    line_type lineE_AB {pointE, directionAB};
    line_type lineZ0 {pointO, directionXY1};
    line_type lineZ1 {pointZ, directionXY1};
    line_type lineZ2 {pointZ, directionXY2};

    ray_type rayAB {pointA, directionAB};
    ray_type rayCD {pointC, directionCD};
    ray_type rayEF {pointE, directionEF};
    ray_type rayDF {pointD, directionDF};

    segment_type segmentAB {pointA, pointB};
    segment_type segmentCD {pointC, pointD};
    segment_type segmentEF {pointE, pointF};

    plane_type planeA {pointA, directionAB};
    plane_type planeB {pointC, directionCD};
    plane_type planeC {pointE, directionEF};

    line_type lineN {{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, 1.0f}};
    line_type lineY {{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
    line_type lineZ {{-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}};

os << "A/B " << geometry::distance(lineAB, lineCD) << endl;
os << "A/C " << geometry::distance(lineAB, lineEF) << endl;
os << "A/N " << geometry::distance(lineAB, lineN) << endl;
os << "Y/Z " << geometry::distance(lineY, lineZ) << endl;

    test(os, "point point intersects", [=]() -> bool { return geometry::intersects(pointA, pointA); }, pointA, pointA);
    test(os, "point point !intersects", [=]() -> bool { return !geometry::intersects(pointA, pointB); }, pointA, pointB);

    // point, line
    test(os, "point line intersects", [=]() -> bool { return geometry::intersects(pointA, lineAB); }, pointA, lineAB);
    test(os, "point line intersects", [=]() -> bool { return geometry::intersects(pointB, lineAB); }, pointB, lineAB);
    test(os, "point line intersects", [=]() -> bool { return geometry::intersects(pointD, lineAB); }, pointD, lineAB);
    test(os, "point line intersects", [=]() -> bool { return geometry::intersects(pointA + 0.1f * vectorAB, lineAB); }, pointA + 0.1f * vectorAB, lineAB);
    test(os, "point line intersects", [=]() -> bool { return geometry::intersects(pointA + 0.9f * vectorAB, lineAB); }, pointA + 0.9f * vectorAB, lineAB);
    test(os, "point line intersects", [=]() -> bool { return geometry::intersects(pointA + vectorAB, lineAB); }, pointA + vectorAB, lineAB);
    test(os, "point line intersects", [=]() -> bool { return geometry::intersects(pointA + 1.1f * vectorAB, lineAB); }, pointA + 1.1f * vectorAB, lineAB);
    test(os, "point line intersects", [=]() -> bool { return geometry::intersects(pointA + 2.0f * vectorAB, lineAB); }, pointA + 2.0f * vectorAB, lineAB);
    test(os, "point line intersects", [=]() -> bool { return geometry::intersects(pointA - 0.1f * vectorAB, lineAB); }, pointA - 0.1f * vectorAB, lineAB);
    test(os, "point line intersects", [=]() -> bool { return geometry::intersects(pointA - vectorAB, lineAB); }, pointA - vectorAB, lineAB);
    test(os, "point line intersects", [=]() -> bool { return geometry::intersects(pointA - 2.0f * vectorAB, lineAB); }, pointA - 2.0f * vectorAB, lineAB);
    test(os, "point line intersects", [=]() -> bool { return geometry::intersects(pointB, lineAB); }, pointB, lineAB);
    test(os, "point line intersects", [=]() -> bool { return geometry::intersects(pointB - 0.1f * vectorAB, lineAB); }, pointB - 0.1f * vectorAB, lineAB);
    test(os, "point line intersects", [=]() -> bool { return geometry::intersects(pointB - 0.9f * vectorAB, lineAB); }, pointB - 0.9f * vectorAB, lineAB);
    test(os, "point line intersects", [=]() -> bool { return geometry::intersects(pointB - vectorAB, lineAB); }, pointB - vectorAB, lineAB);
    test(os, "point line intersects", [=]() -> bool { return geometry::intersects(pointB - 1.1f * vectorAB, lineAB); }, pointB - 1.1f * vectorAB, lineAB);
    test(os, "point line intersects", [=]() -> bool { return geometry::intersects(pointB - 2.0f * vectorAB, lineAB); }, pointB - 2.0f * vectorAB, lineAB);
    test(os, "point line intersects", [=]() -> bool { return geometry::intersects(pointB + 0.1f * vectorAB, lineAB); }, pointB + 0.1f * vectorAB, lineAB);
    test(os, "point line intersects", [=]() -> bool { return geometry::intersects(pointB + vectorAB, lineAB); }, pointB + vectorAB, lineAB);
    test(os, "point line intersects", [=]() -> bool { return geometry::intersects(pointB + 2.0f * vectorAB, lineAB); }, pointB + 2.0f * vectorAB, lineAB);
    test(os, "point line !intersects", [=]() -> bool { return !geometry::intersects(pointN, lineAB); }, pointN, lineAB);

    // point, ray
    test(os, "point ray intersects", [=]() -> bool { return geometry::intersects(pointA, rayAB); }, pointA, rayAB);
    test(os, "point ray intersects", [=]() -> bool { return geometry::intersects(pointA + 0.1f * vectorAB, rayAB); }, pointA + 0.1f * vectorAB, rayAB);
    test(os, "point ray intersects", [=]() -> bool { return geometry::intersects(pointA + 0.9f * vectorAB, rayAB); }, pointA + 0.9f * vectorAB, rayAB);
    test(os, "point ray intersects", [=]() -> bool { return geometry::intersects(pointA + vectorAB, rayAB); }, pointA + vectorAB, rayAB);
    test(os, "point ray intersects", [=]() -> bool { return geometry::intersects(pointA + 1.1f * vectorAB, rayAB); }, pointA + 1.1f * vectorAB, rayAB);
    test(os, "point ray intersects", [=]() -> bool { return geometry::intersects(pointA + 2.0f * vectorAB, rayAB); }, pointA + 2.0f * vectorAB, rayAB);
    test(os, "point ray !intersects", [=]() -> bool { return !geometry::intersects(pointA - 0.1f * vectorAB, rayAB); }, pointA - 0.1f * vectorAB, rayAB);
    test(os, "point ray !intersects", [=]() -> bool { return !geometry::intersects(pointA - vectorAB, rayAB); }, pointA - vectorAB, rayAB);
    test(os, "point ray !intersects", [=]() -> bool { return !geometry::intersects(pointA - 2.0f * vectorAB, rayAB); }, pointA - 2.0f * vectorAB, rayAB);
    test(os, "point ray intersects", [=]() -> bool { return geometry::intersects(pointB, rayAB); }, pointB, rayAB);
    test(os, "point ray intersects", [=]() -> bool { return geometry::intersects(pointB - 0.1f * vectorAB, rayAB); }, pointB - 0.1f * vectorAB, rayAB);
    test(os, "point ray intersects", [=]() -> bool { return geometry::intersects(pointB - 0.9f * vectorAB, rayAB); }, pointB - 0.9f * vectorAB, rayAB);
    test(os, "point ray intersects", [=]() -> bool { return geometry::intersects(pointB - vectorAB, rayAB); }, pointB - vectorAB, rayAB);
    test(os, "point ray !intersects", [=]() -> bool { return !geometry::intersects(pointB - 1.1f * vectorAB, rayAB); }, pointB - 1.1f * vectorAB, rayAB);
    test(os, "point ray !intersects", [=]() -> bool { return !geometry::intersects(pointB - 2.0f * vectorAB, rayAB); }, pointB - 2.0f * vectorAB, rayAB);
    test(os, "point ray intersects", [=]() -> bool { return geometry::intersects(pointB + 0.1f * vectorAB, rayAB); }, pointB + 0.1f * vectorAB, rayAB);
    test(os, "point ray intersects", [=]() -> bool { return geometry::intersects(pointB + vectorAB, rayAB); }, pointB + vectorAB, rayAB);
    test(os, "point ray intersects", [=]() -> bool { return geometry::intersects(pointB + 2.0f * vectorAB, rayAB); }, pointB + 2.0f * vectorAB, rayAB);
    test(os, "point ray !intersects", [=]() -> bool { return !geometry::intersects(pointN, rayAB); }, pointN, rayAB);

    // point, segment
    test(os, "point segment intersects", [=]() -> bool { return geometry::intersects(pointA, segmentAB); }, pointA, segmentAB);
    test(os, "point segment intersects", [=]() -> bool { return geometry::intersects(pointA + 0.1f * vectorAB, segmentAB); }, pointA + 0.1f * vectorAB, segmentAB);
    test(os, "point segment intersects", [=]() -> bool { return geometry::intersects(pointA + 0.9f * vectorAB, segmentAB); }, pointA + 0.9f * vectorAB, segmentAB);
    test(os, "point segment intersects", [=]() -> bool { return geometry::intersects(pointA + vectorAB, segmentAB); }, pointA + vectorAB, segmentAB);
    test(os, "point segment !intersects", [=]() -> bool { return !geometry::intersects(pointA + 1.1f * vectorAB, segmentAB); }, pointA + 1.1f * vectorAB, segmentAB);
    test(os, "point segment !intersects", [=]() -> bool { return !geometry::intersects(pointA + 2.0f * vectorAB, segmentAB); }, pointA + 2.0f * vectorAB, segmentAB);
    test(os, "point segment !intersects", [=]() -> bool { return !geometry::intersects(pointA - 0.1f * vectorAB, segmentAB); }, pointA - 0.1f * vectorAB, segmentAB);
    test(os, "point segment !intersects", [=]() -> bool { return !geometry::intersects(pointA - vectorAB, segmentAB); }, pointA - vectorAB, segmentAB);
    test(os, "point segment !intersects", [=]() -> bool { return !geometry::intersects(pointA - 2.0f * vectorAB, segmentAB); }, pointA - 2.0f * vectorAB, segmentAB);
    test(os, "point segment intersects", [=]() -> bool { return geometry::intersects(pointB, segmentAB); }, pointB, segmentAB);
    test(os, "point segment intersects", [=]() -> bool { return geometry::intersects(pointB - 0.1f * vectorAB, segmentAB); }, pointB - 0.1f * vectorAB, segmentAB);
    test(os, "point segment intersects", [=]() -> bool { return geometry::intersects(pointB - 0.9f * vectorAB, segmentAB); }, pointB - 0.9f * vectorAB, segmentAB);
    test(os, "point segment intersects", [=]() -> bool { return geometry::intersects(pointB - vectorAB, segmentAB); }, pointB - vectorAB, segmentAB);
    test(os, "point segment !intersects", [=]() -> bool { return !geometry::intersects(pointB - 1.1f * vectorAB, segmentAB); }, pointB - 1.1f * vectorAB, segmentAB);
    test(os, "point segment !intersects", [=]() -> bool { return !geometry::intersects(pointB - 2.0f * vectorAB, segmentAB); }, pointB - 2.0f * vectorAB, segmentAB);
    test(os, "point segment !intersects", [=]() -> bool { return !geometry::intersects(pointB + 0.1f * vectorAB, segmentAB); }, pointB + 0.1f * vectorAB, segmentAB);
    test(os, "point segment !intersects", [=]() -> bool { return !geometry::intersects(pointB + vectorAB, segmentAB); }, pointB + vectorAB, segmentAB);
    test(os, "point segment !intersects", [=]() -> bool { return !geometry::intersects(pointB + 2.0f * vectorAB, segmentAB); }, pointB + 2.0f * vectorAB, segmentAB);
    test(os, "point segment !intersects", [=]() -> bool { return !geometry::intersects(pointN, segmentAB); }, pointN, segmentAB);

    // point, plane
    test(os, "point plane intersects", [=]() -> bool { return geometry::intersects(pointA, planeA); }, pointA, planeA);
    test(os, "point plane !intersects", [=]() -> bool { return !geometry::intersects(pointA + 0.1f * vectorAB, planeA); }, pointA + 0.1f * vectorAB, planeA);
    test(os, "point plane !intersects", [=]() -> bool { return !geometry::intersects(pointA + vectorAB, planeA); }, pointA + vectorAB, planeA);
    test(os, "point plane !intersects", [=]() -> bool { return !geometry::intersects(pointA - 0.1f * vectorAB, planeA); }, pointA - 0.1f * vectorAB, planeA);
    test(os, "point plane !intersects", [=]() -> bool { return !geometry::intersects(pointA - vectorAB, planeA); }, pointA - vectorAB, planeA);
    test(os, "point plane intersects", [=]() -> bool { return geometry::intersects(pointP, planeA); }, pointP, planeA);
    test(os, "point plane !intersects", [=]() -> bool { return !geometry::intersects(pointP + 0.1f * vectorAB, planeA); }, pointP + 0.1f * vectorAB, planeA);
    test(os, "point plane !intersects", [=]() -> bool { return !geometry::intersects(pointP + vectorAB, planeA); }, pointP + vectorAB, planeA);
    test(os, "point plane !intersects", [=]() -> bool { return !geometry::intersects(pointP - 0.1f * vectorAB, planeA); }, pointP - 0.1f * vectorAB, planeA);
    test(os, "point plane !intersects", [=]() -> bool { return !geometry::intersects(pointP - vectorAB, planeA); }, pointP - vectorAB, planeA);
    test(os, "point plane !intersects", [=]() -> bool { return !geometry::intersects(pointN, planeA); }, pointN, planeA);

    // line, line
    test(os, "line line intersects", [=]() -> bool { return geometry::intersects(lineAB, lineAB); }, lineAB, lineAB);
    test(os, "line line intersects", [=]() -> bool { return geometry::intersects(lineAB, lineCD); }, lineAB, lineCD);
    test(os, "line line !intersects", [=]() -> bool { return !geometry::intersects(lineAB, lineE_AB); }, lineAB, lineE_AB); // parallel
    test(os, "line line !intersects", [=]() -> bool { return !geometry::intersects(lineAB, lineEF); }, lineAB, lineEF);
    test(os, "line line !intersects", [=]() -> bool { return !geometry::intersects(lineCD, lineEF); }, lineCD, lineEF);

//    test(os, "line line !intersects", [=](const line_type& a, const line_type& b) -> bool { return !geometry::intersects(a, b); }, lineCD, lineEF);

    // line, ray
    test(os, "line ray intersects", [=]() -> bool { return geometry::intersects(lineAB, rayAB); }, lineAB, rayAB);
    test(os, "line ray intersects", [=]() -> bool { return geometry::intersects(lineAB, rayCD); }, lineAB, rayCD);
    test(os, "line ray !intersects", [=]() -> bool { return !geometry::intersects(lineAB, rayEF); }, lineAB, rayEF);
    test(os, "line ray !intersects", [=]() -> bool { return !geometry::intersects(lineCD, rayEF); }, lineCD, rayEF);
    test(os, "line ray !intersects", [=]() -> bool { return !geometry::intersects(lineAB, ray_type(pointD + 0.1f * vectorDC, directionDC)); }, lineAB, ray_type(pointD + 0.1f * vectorDC, directionDC));
    test(os, "line ray !intersects", [=]() -> bool { return !geometry::intersects(lineAB, ray_type(pointD + 0.9f * vectorDC, directionDC)); }, lineAB, ray_type(pointD + 0.9f * vectorDC, directionDC));
    test(os, "line ray !intersects", [=]() -> bool { return !geometry::intersects(lineAB, ray_type(pointD + vectorDC, directionDC)); }, lineAB, ray_type(pointD + vectorDC, directionDC));
    test(os, "line ray !intersects", [=]() -> bool { return !geometry::intersects(lineAB, ray_type(pointD + 1.1f * vectorDC, directionDC)); }, lineAB, ray_type(pointD + 1.1f * vectorDC, directionDC));
    test(os, "line ray !intersects", [=]() -> bool { return !geometry::intersects(lineAB, ray_type(pointD + 2.0f * vectorDC, directionDC)); }, lineAB, ray_type(pointD + 2.0f * vectorDC, directionDC));
    test(os, "line ray intersects", [=]() -> bool { return geometry::intersects(lineAB, ray_type(pointD - 0.1f * vectorDC, directionDC)); }, lineAB, ray_type(pointD - 0.1f * vectorDC, directionDC));
    test(os, "line ray intersects", [=]() -> bool { return geometry::intersects(lineAB, ray_type(pointD - vectorDC, directionDC)); }, lineAB, ray_type(pointD - vectorDC, directionDC));
    test(os, "line ray intersects", [=]() -> bool { return geometry::intersects(lineAB, ray_type(pointD - 2.0f * vectorDC, directionDC)); }, lineAB, ray_type(pointD - 2.0f * vectorDC, directionDC));

    // line, segment
    test(os, "line segment intersects", [=]() -> bool { return geometry::intersects(lineAB, segmentAB); }, lineAB, segmentAB);
    test(os, "line segment intersects", [=]() -> bool { return geometry::intersects(lineAB, segmentCD); }, lineAB, segmentCD);
    test(os, "line segment !intersects", [=]() -> bool { return !geometry::intersects(lineAB, segmentEF); }, lineAB, segmentEF);
    test(os, "line segment !intersects", [=]() -> bool { return !geometry::intersects(lineCD, segmentEF); }, lineCD, segmentEF);
value_type s = (pointD - pointA).norm();
value_type t = (pointC - pointD).norm() / (pointC - (pointD + 0.1f * vectorDC)).norm();
os << "s: " << s << ", t: " << t << endl;
    test(os, "line segment !intersects", [=]() -> bool { return !geometry::intersects(lineAB, segment_type(pointD + 0.1f * vectorDC, pointC)); }, lineAB, segment_type(pointD + 0.1f * vectorDC, pointC));
t = (pointC - pointD).norm() / (pointC - (pointD + 0.9f * vectorDC)).norm();
os << "s: " << s << ", t: " << t << endl;
    test(os, "line segment !intersects", [=]() -> bool { return !geometry::intersects(lineAB, segment_type(pointD + 0.9f * vectorDC, pointC)); }, lineAB, segment_type(pointD + 0.9f * vectorDC, pointC));
t = (pointC - pointD).norm() / (pointC - (pointD + vectorDC)).norm();
os << "s: " << s << ", t: " << t << endl;
    test(os, "line segment !intersects", [=]() -> bool { return !geometry::intersects(lineAB, segment_type(pointD + vectorDC, pointC)); }, lineAB, segment_type(pointD + vectorDC, pointC));
t = (pointC - pointD).norm() / (pointC - (pointD + 1.1f * vectorDC)).norm();
os << "s: " << s << ", t: " << t << endl;
    test(os, "line segment !intersects", [=]() -> bool { return !geometry::intersects(lineAB, segment_type(pointD + 1.1f * vectorDC, pointC)); }, lineAB, segment_type(pointD + 1.1f * vectorDC, pointC));
t = (pointC - pointD).norm() / (pointC - (pointD + 2.0f * vectorDC)).norm();
os << "s: " << s << ", t: " << t << endl;
    test(os, "line segment !intersects", [=]() -> bool { return !geometry::intersects(lineAB, segment_type(pointD + 2.0f * vectorDC, pointC)); }, lineAB, segment_type(pointD + 2.0f * vectorDC, pointC));
t = (pointC - pointD).norm() / (pointC - (pointD - 0.1f * vectorDC)).norm();
os << "s: " << s << ", t: " << t << endl;
    test(os, "line segment intersects", [=]() -> bool { return geometry::intersects(lineAB, segment_type(pointD - 0.1f * vectorDC, pointC)); }, lineAB, segment_type(pointD - 0.1f * vectorDC, pointC));
t = (pointC - pointD).norm() / (pointC - (pointD - vectorDC)).norm();
os << "s: " << s << ", t: " << t << endl;
    test(os, "line segment intersects", [=]() -> bool { return geometry::intersects(lineAB, segment_type(pointD - vectorDC, pointC)); }, lineAB, segment_type(pointD - vectorDC, pointC));
t = (pointC - pointD).norm() / (pointC - (pointD - 2.0f * vectorDC)).norm();
os << "s: " << s << ", t: " << t << endl;
    test(os, "line segment intersects", [=]() -> bool { return geometry::intersects(lineAB, segment_type(pointD - 2.0f * vectorDC, pointC)); }, lineAB, segment_type(pointD - 2.0f * vectorDC, pointC));

    test(os, "line segment intersects", [=]() -> bool { return geometry::intersects(lineAB, segment_type(pointC, pointD + 0.1f * vectorCD)); }, lineAB, segment_type(pointC, pointD + 0.1f * vectorCD));
    test(os, "line segment intersects", [=]() -> bool { return geometry::intersects(lineAB, segment_type(pointC, pointD + 0.9f * vectorCD)); }, lineAB, segment_type(pointC, pointD + 0.9f * vectorCD));
    test(os, "line segment intersects", [=]() -> bool { return geometry::intersects(lineAB, segment_type(pointC, pointD + vectorCD)); }, lineAB, segment_type(pointC, pointD + vectorCD));
    test(os, "line segment intersects", [=]() -> bool { return geometry::intersects(lineAB, segment_type(pointC, pointD + 1.1f * vectorCD)); }, lineAB, segment_type(pointC, pointD + 1.1f * vectorCD));
    test(os, "line segment intersects", [=]() -> bool { return geometry::intersects(lineAB, segment_type(pointC, pointD + 2.0f * vectorCD)); }, lineAB, segment_type(pointC, pointD + 2.0f * vectorCD));
    test(os, "line segment !intersects", [=]() -> bool { return !geometry::intersects(lineAB, segment_type(pointC, pointD - 0.1f * vectorCD)); }, lineAB, segment_type(pointC, pointD - 0.1f * vectorCD));
    test(os, "line segment !intersects", [=]() -> bool { return !geometry::intersects(lineAB, segment_type(pointC, pointD - vectorCD)); }, lineAB, segment_type(pointC, pointD - vectorCD));
    test(os, "line segment !intersects", [=]() -> bool { return !geometry::intersects(lineAB, segment_type(pointC, pointD - 2.0f * vectorCD)); }, lineAB, segment_type(pointC, pointD - 2.0f * vectorCD));

    // ray, segment
    // ...

    value_test(os, "point line distance", [=]() -> value_type { return geometry::distance(lineZ0, pointO); }, 0.0f, lineZ0, pointZ);
    value_test(os, "point line distance", [=]() -> value_type { return geometry::distance(lineZ0, pointZ); }, 5.0f, lineZ0, pointZ);
    value_test(os, "point line distance", [=]() -> value_type { return geometry::distance(lineZ1, pointO); }, 5.0f, lineZ1, pointZ);
    value_test(os, "point line distance", [=]() -> value_type { return geometry::distance(lineZ1, pointZ); }, 0.0f, lineZ1, pointZ);

    value_test(os, "point line distance", [=]() -> value_type { return geometry::distance(lineAB, pointA); }, 0.0f, lineAB, pointA);
    value_test(os, "point line distance", [=]() -> value_type { return geometry::distance(lineAB, pointB); }, 0.0f, lineAB, pointB);
    value_test(os, "point line distance", [=]() -> value_type { return geometry::distance(lineCD, pointC); }, 0.0f, lineCD, pointC);
    value_test(os, "point line distance", [=]() -> value_type { return geometry::distance(lineCD, pointD); }, 0.0f, lineCD, pointD);
    value_test(os, "point line distance", [=]() -> value_type { return geometry::distance(lineEF, pointE); }, 0.0f, lineEF, pointE);
    value_test(os, "point line distance", [=]() -> value_type { return geometry::distance(lineEF, pointF); }, 0.0f, lineEF, pointF);
    value_test(os, "point line distance", [=]() -> value_type { return geometry::distance(lineAB, pointO); }, 0.0f, lineAB, pointO);
    value_test(os, "point line distance", [=]() -> value_type { return geometry::distance(lineAB, pointC); }, 2.61861f, lineAB, pointC);
    value_test(os, "point line distance", [=]() -> value_type { return geometry::distance(lineCD, pointE); }, 2.23607f, lineCD, pointE);
    value_test(os, "line line distance", [=]() -> value_type { return geometry::distance(lineAB, lineCD); }, 0.0f, lineAB, lineCD);
    value_test(os, "line line distance", [=]() -> value_type { return geometry::distance(lineCD, lineEF); }, 0.57735f, lineCD, lineEF);
    value_test(os, "line line distance", [=]() -> value_type { return geometry::distance(lineEF, lineAB); }, 1.60591f, lineEF, lineAB);
    value_test(os, "line line distance", [=]() -> value_type { return geometry::distance(lineAB, lineZ2); }, 2.88675f, lineAB, lineZ2);

    value_test(os, "line line distance", [=]() -> value_type { return geometry::distance(lineZ0, lineZ1); }, 5.0f, lineZ0, lineZ1);
    value_test(os, "line line distance", [=]() -> value_type { return geometry::distance(lineZ1, lineZ2); }, 0.0f, lineZ1, lineZ2);

    // ...

    segment_type segmentT({0.0f, 0.0f, -10.0f}, {2.82172370f, 0.0f, -9.24392128f});
    polygon_type polygonT {{0.0f, 0.0f, -7.5f}, {-6.70820379f, 0.0f, -3.35410190f}, {-2.07294893f, 6.37988091f, -3.35410190f}};
    test(os, "segment polygon !intersects", [=]() -> bool { return !geometry::intersects(segmentT, polygonT); }, segmentT, polygonT);

    validator.report_results();
    return 0;
}
