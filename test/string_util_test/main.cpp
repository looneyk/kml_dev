//------------------------------------------------------------------------------
// string_util_test
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/common/KeyValueStore.hpp"
#include "quetzal/common/string_util.hpp"
#include "quetzal/geometry/AxisAlignedBoundingBox.hpp"
#include "quetzal/geometry/BoundingSphere.hpp"
#include "quetzal/geometry/Box.hpp"
#include "quetzal/geometry/Circle.hpp"
#include "quetzal/geometry/DirectedPoint.hpp"
#include "quetzal/geometry/DiscreteBox.hpp"
#include "quetzal/geometry/DiscreteDirectedPoint.hpp"
#include "quetzal/geometry/DiscretePoint.hpp"
#include "quetzal/geometry/HalfPlane.hpp"
#include "quetzal/geometry/HalfSpace.hpp"
#include "quetzal/geometry/Intersection.hpp"
#include "quetzal/geometry/Line.hpp"
#include "quetzal/geometry/Locus.hpp"
#include "quetzal/geometry/Orientation.hpp"
#include "quetzal/geometry/OrientedPoint.hpp"
#include "quetzal/geometry/Plane.hpp"
#include "quetzal/geometry/Point.hpp"
#include "quetzal/geometry/Polygon.hpp"
#include "quetzal/geometry/Ray.hpp"
#include "quetzal/geometry/Segment.hpp"
#include "quetzal/geometry/Sphere.hpp"
#include "quetzal/math/CartesianTraits.hpp"
#include "quetzal/math/DiscreteVector.hpp"
#include "quetzal/math/HomogeneousTraits.hpp"
#include "quetzal/math/Interval.hpp"
#include "quetzal/math/Vector.hpp"
#include "quetzal/testing/named_checks.hpp"
#include <doctest/doctest.h>
#include <format>
#include <optional>
#include <sstream>
#include <string>

using namespace std;
using namespace quetzal;

namespace
{

    //--------------------------------------------------------------------------
    using cartesian_traits_type = math::CartesianTraits<double, 3>;
    using homogeneous_traits_type = math::HomogeneousTraits<double, 3>;

    using vector_type = math::Vector<cartesian_traits_type>;
    using homogeneous_vector_type = math::Vector<homogeneous_traits_type>;
    using point_type = geometry::Point<cartesian_traits_type>;
    using homogeneous_point_type = geometry::Point<homogeneous_traits_type>;

    using discrete_vector_type = math::DiscreteVector<long, 3>;
    using discrete_point_type = geometry::DiscretePoint<long, 3>;
    using discrete_box_type = geometry::DiscreteBox<long, 2>;

    using count_interval_type = math::Interval<size_t>;
    using integer_interval_type = math::Interval<int>;
    using real_interval_type = math::Interval<double>;
    using bounds_type = geometry::AxisAlignedBoundingBox<cartesian_traits_type>;

    //--------------------------------------------------------------------------
    // What to_chars wrote, or a marker identifying the error
    template<typename T>
    string written(const T& t)
    {
        char buffer[128] {};
        auto result = to_chars(buffer, buffer + sizeof buffer, t);

        if (result.ec == errc::value_too_large)
        {
            return "<too large>";
        }

        return result.ec == errc {} ? string(buffer, result.ptr) : "<error>";
    }

    //--------------------------------------------------------------------------
    // Writes the value to a stream and reads it back from another, both with the same separator, brackets, and series
    // delimiter. An empty separator or series delimiter is not set on the streams, so the type's default applies. Returns
    // std::nullopt if the read fails
    template<typename T>
    optional<T> read_back(const T& t, const string& separator, const string& opening_bracket = "", const string& closing_bracket = "",
        const string& series_delimiter = "")
    {
        ostringstream os;
        if (!separator.empty())
        {
            os << separator_delim(separator);
        }

        if (!series_delimiter.empty())
        {
            os << series_delim(series_delimiter);
        }

        os << brackets_delim(opening_bracket, closing_bracket) << t;

        istringstream is(os.str());
        if (!separator.empty())
        {
            is >> separator_delim(separator);
        }

        if (!series_delimiter.empty())
        {
            is >> series_delim(series_delimiter);
        }

        T read;
        is >> brackets_delim(opening_bracket, closing_bracket) >> read;
        return is.fail() ? nullopt : make_optional(read);
    }

    //--------------------------------------------------------------------------
    TEST_CASE("skip_whitespace")
    {
        string s = "  \t x";
        CHECK_CASE("spaces and tabs are skipped", skip_whitespace(s.data(), s.data() + s.size()) == s.data() + 4);

        string none = "x  ";
        CHECK_CASE("a non-blank first position is returned as it is", skip_whitespace(none.data(), none.data() + none.size()) == none.data());

        string blank = "   ";
        CHECK_CASE("all blank reaches the end", skip_whitespace(blank.data(), blank.data() + blank.size()) == blank.data() + blank.size());

        string empty;
        CHECK_CASE("an empty range is returned as it is", skip_whitespace(empty.data(), empty.data()) == empty.data());

        // Only space and tab count, so a line break stops the skip
        string newline = " \n ";
        CHECK_CASE("a line break is not skipped", skip_whitespace(newline.data(), newline.data() + newline.size()) == newline.data() + 1);

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("skip_delimiter")
    {
        {
            istringstream is("  (< rest");
            skip_delimiter(is >> ws, "(<");
            string rest;
            is >> rest;
            CHECK_CASE("a bracket is consumed past leading whitespace", is.good() || is.eof());
            CHECK_CASE_EQUAL("and what follows it is left", rest, "rest");
        }

        {
            istringstream is("(x");
            skip_delimiter(is, "(<");
            CHECK_CASE("a bracket only partly present sets failbit", is.fail());
        }

        {
            istringstream is(", rest");
            skip_delimiter(is, ",");
            string rest;
            is >> rest;
            CHECK_CASE_EQUAL("the separator is consumed", rest, "rest");
        }

        {
            istringstream is("rest");
            skip_delimiter(is, "");
            string rest;
            is >> rest;
            CHECK_CASE("an empty pattern skips nothing and succeeds", !is.fail());
            CHECK_CASE_EQUAL("leaving the stream where it was", rest, "rest");
        }

        {
            istringstream is("x");
            skip_delimiter(is, ",");
            CHECK_CASE("a missing separator sets failbit", is.fail());
        }

        return;
    }

    //--------------------------------------------------------------------------
    // The one pair of conversions all four vector and point types go through
    TEST_CASE("array_from_chars")
    {
        {
            discrete_vector_type v;
            string s = "1 -2 30";
            auto result = from_chars(s.data(), s.data() + s.size(), v);
            CHECK_CASE("a discrete vector reads every component", result.ec == errc {} && result.ptr == s.data() + s.size());
            CHECK_CASE("with the values the string names", v == discrete_vector_type(1L, -2L, 30L));
        }

        {
            discrete_vector_type v;
            string s = "  4\t 5   6";
            auto result = from_chars(s.data(), s.data() + s.size(), v);
            CHECK_CASE("leading and inner whitespace is skipped", result.ec == errc {} && v == discrete_vector_type(4L, 5L, 6L));
        }

        {
            discrete_vector_type v;
            string s = "7 x 9";
            auto result = from_chars(s.data(), s.data() + s.size(), v);
            CHECK_CASE("a component that cannot be read is an error", result.ec == errc::invalid_argument);
            CHECK_CASE("reported where it stopped", result.ptr == s.data() + 2);
        }

        {
            discrete_vector_type v;
            string s = "1 2";
            auto result = from_chars(s.data(), s.data() + s.size(), v);
            CHECK_CASE("a string of too few components is an error", result.ec != errc {});
        }

        {
            discrete_point_type p;
            string s = " 11 -22\t33";
            auto result = from_chars(s.data(), s.data() + s.size(), p);
            CHECK_CASE("a discrete point reads every component",
                result.ec == errc {} && p.x() == 11L && p.y() == -22L && p.z() == 33L);
        }

        {
            vector_type v;
            string s = "1.5 -2.5 3";
            auto result = from_chars(s.data(), s.data() + s.size(), v);
            CHECK_CASE("a cartesian vector reads every component",
                result.ec == errc {} && v.x() == 1.5 && v.y() == -2.5 && v.z() == 3.0);
        }

        {
            point_type p;
            string s = "1.5 -2.5 3";
            auto result = from_chars(s.data(), s.data() + s.size(), p);
            CHECK_CASE("a cartesian point reads every component",
                result.ec == errc {} && p.x() == 1.5 && p.y() == -2.5 && p.z() == 3.0);
        }

        // Homogeneous coordinates are written as dimension components; the caller supplies the weight, which
        // distinguishes a direction from a position
        {
            homogeneous_vector_type v;
            string s = "1.5 -2.5 3";
            auto result = from_chars(s.data(), s.data() + s.size(), v);
            CHECK_CASE("a homogeneous vector reads three and concludes a weight of 0",
                result.ec == errc {} && result.ptr == s.data() + s.size() && v.weight() == 0.0);
        }

        {
            homogeneous_point_type p;
            string s = "1.5 -2.5 3";
            auto result = from_chars(s.data(), s.data() + s.size(), p);
            CHECK_CASE("a homogeneous point reads three and concludes a weight of 1",
                result.ec == errc {} && result.ptr == s.data() + s.size() && p.weight() == 1.0);
        }

        return;
    }

    //--------------------------------------------------------------------------
    // Reading matches the separator between components exactly
    TEST_CASE("separators")
    {
        {
            discrete_vector_type v;
            string s = "16,10,4";
            auto result = math::from_chars(s.data(), s.data() + s.size(), v, {.separator = ","});
            CHECK_CASE("a separator reads the components it separates",
                result.ec == errc {} && result.ptr == s.data() + s.size() && v == discrete_vector_type(16L, 10L, 4L));
        }

        {
            discrete_vector_type v;
            string s = "16 10 4";
            auto result = math::from_chars(s.data(), s.data() + s.size(), v, {.separator = ","});
            CHECK_CASE("and rejects a string the separator is absent from", result.ec == errc::invalid_argument);
            CHECK_CASE("reporting where it expected one", result.ptr == s.data() + 2);
        }

        {
            char buffer[64];
            discrete_vector_type v(16L, 10L, 4L);
            auto result = math::to_chars(buffer, buffer + sizeof buffer, v, {.separator = ","});
            CHECK_CASE("writing uses the separator given", string(buffer, result.ptr) == "16,10,4");
        }

        {
            char buffer[64];
            discrete_vector_type v(7L, -3L, 0L);
            discrete_vector_type w;
            auto written = math::to_chars(buffer, buffer + sizeof buffer, v, {.separator = ", "});
            auto read = math::from_chars(buffer, written.ptr, w, {.separator = ", "});
            CHECK_CASE("a multi-character separator round trips",
                read.ec == errc {} && read.ptr == written.ptr && w == v);
        }

        {
            point_type p;
            string s = "1.5,-2.5,3";
            auto result = geometry::from_chars(s.data(), s.data() + s.size(), p, {.separator = ","});
            CHECK_CASE("a cartesian point takes one too",
                result.ec == errc {} && p.x() == 1.5 && p.y() == -2.5 && p.z() == 3.0);
        }

        {
            istringstream is("16,10,4");
            discrete_vector_type v;
            is >> separator_delim(",") >> v;
            CHECK_CASE("a stream reads with the separator the manipulator sets",
                !is.fail() && v == discrete_vector_type(16L, 10L, 4L));
        }

        {
            ostringstream os;
            os << separator_delim(",") << discrete_vector_type(16L, 10L, 4L);
            CHECK_CASE("and writes the same", os.str() == "16,10,4");
        }

        {
            istringstream is("1,2,3");
            vector_type v;
            is >> separator_delim(",") >> v;
            CHECK_CASE("a traits vector reads every component past the first",
                !is.fail() && v.x() == 1.0 && v.y() == 2.0 && v.z() == 3.0);
        }

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("array_to_chars")
    {
        CHECK_CASE_EQUAL("a discrete vector writes its components", written(discrete_vector_type(1L, -2L, 30L)), "1 -2 30");
        CHECK_CASE_EQUAL("a discrete point writes its components", written(discrete_point_type(11L, -22L, 33L)), "11 -22 33");
        CHECK_CASE_EQUAL("a cartesian vector writes its components", written(vector_type(1.5, -2.5, 3.0)), "1.5 -2.5 3");
        CHECK_CASE_EQUAL("a cartesian point writes its components", written(point_type(1.5, -2.5, 3.0)), "1.5 -2.5 3");
        CHECK_CASE_EQUAL("a homogeneous vector writes the dimension, not the weight", written(homogeneous_vector_type(1.5, -2.5, 3.0)), "1.5 -2.5 3");
        CHECK_CASE_EQUAL("a homogeneous point writes the dimension, not the weight", written(homogeneous_point_type(1.5, -2.5, 3.0)), "1.5 -2.5 3");

        // A weighted point is reduced before it is written, so what comes out is where it is
        {
            homogeneous_point_type p(6.0, -10.0, 14.0);
            p.rep()[3] = 2.0;
            CHECK_CASE_EQUAL("a weighted homogeneous point is reduced before writing", written(p), "3 -5 7");
        }

        CHECK_CASE_EQUAL("a negative zero is written as zero", written(vector_type(-0.0, 1.0, 2.0)), "0 1 2");

        {
            char buffer[4] {};
            auto result = to_chars(buffer, buffer + sizeof buffer, discrete_vector_type(100L, 200L, 300L));
            CHECK_CASE("a buffer too small for a discrete vector is an error", result.ec == errc::value_too_large);
        }

        {
            char buffer[4] {};
            auto result = to_chars(buffer, buffer + sizeof buffer, vector_type(100.0, 200.0, 300.0));
            CHECK_CASE("a buffer too small for a cartesian vector is an error", result.ec == errc::value_too_large);
        }

        // The delimiter is not written where there is no room left for it
        {
            char buffer[3] {};
            auto result = array_to_chars(buffer, buffer + sizeof buffer, discrete_vector_type(1L, 2L, 3L), {.separator = ", "});
            CHECK_CASE("a delimiter that will not fit is an error", result.ec == errc::value_too_large);
        }

        CHECK_CASE_EQUAL("a delimiter other than a space is written",
            [] { char buffer[32] {}; auto r = array_to_chars(buffer, buffer + sizeof buffer, discrete_vector_type(1L, 2L, 3L), {.separator = ", "});
                return r.ec == errc {} ? string(buffer, r.ptr) : string("<error>"); }(), "1, 2, 3");

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("round trip")
    {
        {
            char buffer[64] {};
            auto written_result = to_chars(buffer, buffer + sizeof buffer, discrete_vector_type(-7L, 0L, 123L));
            discrete_vector_type v;
            auto read_result = from_chars(buffer, written_result.ptr, v);
            CHECK_CASE("a discrete vector survives a round trip",
                read_result.ec == errc {} && v == discrete_vector_type(-7L, 0L, 123L));
        }

        {
            char buffer[64] {};
            auto written_result = to_chars(buffer, buffer + sizeof buffer, discrete_point_type(-7L, 0L, 123L));
            discrete_point_type p;
            auto read_result = from_chars(buffer, written_result.ptr, p);
            CHECK_CASE("a discrete point survives a round trip",
                read_result.ec == errc {} && p.x() == -7L && p.y() == 0L && p.z() == 123L);
        }

        {
            char buffer[64] {};
            auto written_result = to_chars(buffer, buffer + sizeof buffer, vector_type(-7.25, 0.0, 123.5));
            vector_type v;
            auto read_result = from_chars(buffer, written_result.ptr, v);
            CHECK_CASE("a cartesian vector survives a round trip",
                read_result.ec == errc {} && v.x() == -7.25 && v.y() == 0.0 && v.z() == 123.5);
        }

        {
            char buffer[64] {};
            auto written_result = to_chars(buffer, buffer + sizeof buffer, homogeneous_point_type(-7.25, 0.0, 123.5));
            homogeneous_point_type p;
            auto read_result = from_chars(buffer, written_result.ptr, p);
            CHECK_CASE("a homogeneous point survives a round trip with its weight restored",
                read_result.ec == errc {} && p.x() == -7.25 && p.z() == 123.5 && p.weight() == 1.0);
        }

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("stream round trip")
    {
        const vector_type v(-7.25, 0.0, 123.5);
        const point_type p(1.5, -2.5, 3.0);
        const homogeneous_vector_type hv(-7.25, 0.0, 123.5);
        const homogeneous_point_type hp(1.5, -2.5, 3.0);
        const discrete_vector_type dv(-7L, 0L, 123L);
        const discrete_point_type dp(16L, -10L, 4L);
        const discrete_box_type box(geometry::DiscretePoint<long, 2>(1L, -2L), math::DiscreteVector<long, 2>(3L, 4L));

        CHECK_CASE("a cartesian vector reads back what a stream wrote", read_back(v, "") == v);
        CHECK_CASE("with a separator and brackets", read_back(v, ", ", "[", "]") == v && read_back(v, ",", "(", ")") == v);
        CHECK_CASE("and with brackets of more than one character", read_back(v, ", ", "<< ", " >>") == v);
        CHECK_CASE("a cartesian point too", read_back(p, "") == p && read_back(p, ", ", "[", "]") == p);
        CHECK_CASE("a homogeneous vector reads back its components with weight 0", read_back(hv, "") == hv
            && read_back(hv, ", ", "[", "]") == hv && read_back(hv, ", ", "[", "]")->weight() == 0.0);
        CHECK_CASE("a homogeneous point reads back its components with weight 1", read_back(hp, "") == hp
            && read_back(hp, ", ", "(", ")") == hp && read_back(hp, ", ", "(", ")")->weight() == 1.0);
        CHECK_CASE("a discrete vector reads back what a stream wrote", read_back(dv, "") == dv && read_back(dv, ", ", "[", "]") == dv);
        CHECK_CASE("a discrete point too", read_back(dp, "") == dp && read_back(dp, ",", "(", ")") == dp);
        CHECK_CASE("a discrete box reads back with a separator and brackets", read_back(box, ",") == box && read_back(box, ", ", "[", "]") == box);

        const math::Vector<math::CartesianTraits<double, 1>> v1(-7.25);
        const geometry::Point<math::HomogeneousTraits<double, 1>> hp1(1.5);
        CHECK_CASE("a one-dimensional vector and point read back with brackets", read_back(v1, ", ", "[", "]") == v1
            && read_back(hp1, ", ", "(", ")") == hp1 && read_back(hp1, ", ", "(", ")")->weight() == 1.0);

        {
            istringstream is("[-7.25] 5");
            math::Vector<math::CartesianTraits<double, 1>> read;
            double next = 0.0;
            is >> brackets_delim("[", "]") >> read >> next;
            CHECK_CASE("and the closing bracket is skipped, so the stream continues after it", !is.fail() && read == v1 && next == 5.0);
        }

        {
            istringstream is("1, 2, 3]");
            vector_type read;
            is >> separator_delim(", ") >> brackets_delim("[", "]") >> read;
            CHECK_CASE("a stream with brackets set fails on a value without its opening bracket", is.fail());
        }

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("to_type and to_bool")
    {
        CHECK_CASE("an integer is read", to_type<int>("42") == 42);
        CHECK_CASE("a negative integer is read", to_type<long>("-17") == -17);
        CHECK_CASE("a real is read", to_type<double>("2.5") == 2.5);
        CHECK_CASE_EQUAL("a string is returned as it is", to_type<string>("text"), "text");
        CHECK_CASE("true is read", to_type<bool>("true"));
        CHECK_CASE("1 is read as true", to_bool("1"));
        CHECK_CASE("false is read", !to_bool("false"));
        CHECK_CASE("0 is read as false", !to_bool("0"));

        {
            bool threw = false;
            try
            {
                to_bool("yes");
            }
            catch (const runtime_error&)
            {
                threw = true;
            }
            CHECK_CASE("a value that is neither is rejected", threw);
        }

        CHECK_CASE("try_type reads what it can", try_type<int>("42") == 42);
        CHECK_CASE("try_type reports what it cannot", !try_type<int>("x").has_value());

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("to_string")
    {
        CHECK_CASE_EQUAL("a string is returned as it is", to_string(string("text")), "text");
        CHECK_CASE_EQUAL("a wide string is converted", to_string(wstring(L"text")), "text");
        CHECK_CASE_EQUAL("true", to_string(true), "true");
        CHECK_CASE_EQUAL("false", to_string(false), "false");
        CHECK_CASE_EQUAL("an array type is space separated", to_string(vector_type(1.0, 2.0, 3.0)), "1 2 3");
        CHECK_CASE_EQUAL("an optional with a value", to_string(optional<int>(7)), "7");
        CHECK_CASE_EQUAL("an optional without one", to_string(optional<int>()), "nullopt");

        return;
    }

    //--------------------------------------------------------------------------
    // The manipulators and the format spec they produce are what put operator<< and
    // std::format in agreement over an array type
    TEST_CASE("separator_delim and brackets_delim")
    {
        {
            ostringstream os;
            os << vector_type(1.0, 2.0, 3.0);
            CHECK_CASE_EQUAL("the default is space separated and unbracketed", os.str(), "1 2 3");
        }

        {
            ostringstream os;
            os << separator_delim(", ") << brackets_delim("[", "]") << vector_type(1.0, 2.0, 3.0);
            CHECK_CASE_EQUAL("the manipulators separate and bracket", os.str(), "[1, 2, 3]");
        }

        {
            ostringstream os;
            os << separator_delim(", ") << brackets_delim("[", "]") << brackets_delim("", "") << vector_type(1.0, 2.0, 3.0);
            CHECK_CASE_EQUAL("empty brackets remove the brackets", os.str(), "1, 2, 3");
        }

        {
            ostringstream os;
            os << separator_delim(", ") << separator_delim("") << discrete_vector_type(2026L, 10L, 25L);
            CHECK_CASE_EQUAL("an empty separator sets an empty separator", os.str(), "20261025");
        }

        {
            ostringstream os;
            os << series_delim("") << integer_interval_type(3, 7);
            CHECK_CASE_EQUAL("and an empty series delimiter an empty series delimiter", os.str(), "37");
        }

        {
            ostringstream os;
            os << separator_delim(" | ") << brackets_delim("<", ">") << vector_type(1.0, 2.0, 3.0);
            CHECK_CASE_EQUAL("a stream writes any separator and brackets, including those a format spec cannot represent", os.str(),
                "<1 | 2 | 3>");
        }

        {
            ostringstream os;
            os << separator_delim("_") << vector_type(1.0, 2.0, 3.0);
            CHECK_CASE_EQUAL("an underscore is written as itself", os.str(), "1_2_3");
        }

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("delimiter format spec")
    {
        const vector_type v(1.5, 2.25, 3.0);
        const discrete_vector_type dv(-7L, 0L, 123L);
        const bounds_type box(point_type(1.0, 2.0, 3.0), point_type(4.0, 5.0, 6.0));

        CHECK_CASE_EQUAL("a spec that does not begin with '/' is the element type's spec alone", std::format("{:.2f}", v), "1.50 2.25 3.00");
        CHECK_CASE_EQUAL("a delimiter part sets the separator and the brackets", std::format("{:/s, /o[/c]}", v), "[1.5, 2.25, 3]");
        CHECK_CASE_EQUAL("in any order", std::format("{:/c>/s;/o<}", dv), "<-7;0;123>");
        CHECK_CASE_EQUAL("and the element type's spec follows the first ':'", std::format("{:/s, /o[/c]:.2f}", v), "[1.50, 2.25, 3.00]");
        CHECK_CASE_EQUAL("brackets of more than one character", std::format("{:/o<< /c >>}", dv), "<< -7 0 123 >>");
        CHECK_CASE_EQUAL("'/b' sets a pair of single-character brackets", std::format("{:/s, /b[]}", dv), "[-7, 0, 123]");
        CHECK_CASE_EQUAL("which may be escaped", std::format("{:/b/{/}}", dv), "{-7 0 123}");
        CHECK_CASE_EQUAL("an empty separator", std::format("{:/s}", dv), "-70123");
        CHECK_CASE_EQUAL("a component left out keeps its default", std::format("{:/o(/c)}", dv), "(-7 0 123)");
        CHECK_CASE_EQUAL("'/' escapes ':', '/', '{', and '}'", std::format("{:/s /: /o/{/c/}}", dv), "{-7 : 0 : 123}");
        CHECK_CASE_EQUAL("and any character that is not a prefix letter", std::format("{:/s//x}", dv), "-7/x0/x123");
        CHECK_CASE_EQUAL("an array type ignores the series delimiter", std::format("{:/n../s,}", dv), "-7,0,123");
        CHECK_CASE_EQUAL("a series type sets it, and its elements' separator and brackets", std::format("{:/n../s, /o[/c]}", box),
            "[1, 2, 3]..[4, 5, 6]");
        CHECK_CASE_EQUAL("and its elements' spec follows the first ':'", std::format("{:/n to :.1f}", box), "1.0 2.0 3.0 to 4.0 5.0 6.0");
        CHECK_CASE_EQUAL("an empty delimiter part leaves every default", std::format("{::.1f}", real_interval_type(1.5, 2.25)), "1.5 .. 2.2");

        CHECK_THROWS_AS((void)std::vformat("{:/x,}", std::make_format_args(dv)), std::format_error);
        CHECK_THROWS_AS((void)std::vformat("{:/s,/s;}", std::make_format_args(dv)), std::format_error);
        CHECK_THROWS_AS((void)std::vformat("{:/s,/}", std::make_format_args(dv)), std::format_error);
        CHECK_THROWS_AS((void)std::vformat("{:/b[}", std::make_format_args(dv)), std::format_error);
        CHECK_THROWS_AS((void)std::vformat("{:/b[]]}", std::make_format_args(dv)), std::format_error);
        CHECK_THROWS_AS((void)std::vformat("{:/b[]/o<}", std::make_format_args(dv)), std::format_error);

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("series")
    {
        auto bounds_are = []<typename T>(const math::Interval<T>& interval, T lower, T upper)
        {
            return interval.lower() == lower && interval.upper() == upper;
        };

        const bounds_type box(point_type(1.0, 2.0, 3.0), point_type(4.0, 5.0, 6.0));

        CHECK_CASE_EQUAL("an interval's bounds are written with the default delimiter", std::format("{}", count_interval_type(2, 5)), "2 .. 5");
        CHECK_CASE_EQUAL("or with the series delimiter the spec sets", std::format("{:/n..}", count_interval_type(2, 5)), "2..5");
        CHECK_CASE_EQUAL("where a space is literal", std::format("{:/n to }", integer_interval_type(-3, 7)), "-3 to 7");
        CHECK_CASE_EQUAL("and a separator with spaces too", std::format("{:/n../o[/c]/s x }", box), "[1 x 2 x 3]..[4 x 5 x 6]");
        CHECK_CASE_EQUAL("and the rest of the spec formats each bound", std::format("{:/n..:.2f}", real_interval_type(1.5, 2.25)), "1.50..2.25");
        CHECK_CASE_EQUAL("negative zero is written as zero", std::format("{}", real_interval_type(-0.0, 1.0)), "0 .. 1");
        CHECK_CASE_EQUAL("a bounding box's bounds are written the same way", std::format("{}", box), "1 2 3 .. 4 5 6");
        CHECK_CASE_EQUAL("with its points' delimiters", std::format("{:/n../s, /o[/c]}", box), "[1, 2, 3]..[4, 5, 6]");

        {
            count_interval_type interval;
            const string s = "2..5";
            auto result = from_chars(s.data(), s.data() + s.size(), interval);
            CHECK_CASE("an interval reads its bounds with no space around the delimiter",
                result.ec == errc {} && result.ptr == s.data() + s.size() && bounds_are(interval, size_t(2), size_t(5)));
        }

        {
            integer_interval_type interval;
            const string s = " -3 .. 7";
            auto result = from_chars(s.data(), s.data() + s.size(), interval);
            CHECK_CASE("or with spaces, and negative bounds", result.ec == errc {} && bounds_are(interval, -3, 7));
        }

        {
            real_interval_type interval;
            const string s = "1.5 .. 2.25";
            auto result = from_chars(s.data(), s.data() + s.size(), interval);
            CHECK_CASE("and real bounds", result.ec == errc {} && bounds_are(interval, 1.5, 2.25));
        }

        {
            real_interval_type interval;
            const string s = "2..5";
            auto result = from_chars(s.data(), s.data() + s.size(), interval);
            CHECK_CASE("a real lower bound stops at the delimiter rather than reading its '.' as a decimal point",
                result.ec == errc {} && bounds_are(interval, 2.0, 5.0));
        }

        {
            count_interval_type interval(1, 1);
            const string s = "2-5";
            auto result = from_chars(s.data(), s.data() + s.size(), interval);
            CHECK_CASE("a missing delimiter fails, leaving the interval unchanged",
                result.ec == errc::invalid_argument && bounds_are(interval, size_t(1), size_t(1)));
        }

        {
            count_interval_type interval;
            const string s = "-3 .. 5";
            auto result = from_chars(s.data(), s.data() + s.size(), interval);
            CHECK_CASE("a negative bound fails for an unsigned interval", result.ec != errc {});
        }

        {
            const KeyValueStore tokens {{"stairs_down", "1..2"}, {"stairs_up", "0 .. 3"}};
            CHECK_CASE("a key-value store reads an interval in either form",
                bounds_are(*tokens.value<count_interval_type>("stairs_down"), size_t(1), size_t(2))
                && bounds_are(*tokens.value<count_interval_type>("stairs_up"), size_t(0), size_t(3)));
        }

        {
            char buffer[64] {};
            auto written_result = to_chars(buffer, buffer + sizeof buffer, integer_interval_type(-3, 7), {.series_delimiter = ".."});
            integer_interval_type interval;
            auto read_result = from_chars(buffer, written_result.ptr, interval, {.series_delimiter = ".."});
            CHECK_CASE_EQUAL("an interval's to_chars writes the delimiter it is passed", string(buffer, written_result.ptr), "-3..7");
            CHECK_CASE("and from_chars reads it back", read_result.ec == errc {} && bounds_are(interval, -3, 7));
        }

        {
            char buffer[64] {};
            const FormatSpec spec {.separator = ",", .series_delimiter = ".."};
            auto written_result = to_chars(buffer, buffer + sizeof buffer, box, spec);
            bounds_type read;
            auto read_result = from_chars(buffer, written_result.ptr, read, spec);
            CHECK_CASE_EQUAL("a bounding box's to_chars writes the separator and the series delimiter", string(buffer, written_result.ptr),
                "1,2,3..4,5,6");
            CHECK_CASE("and from_chars reads it back", read_result.ec == errc {} && read == box);
        }

        {
            char buffer[4] {};
            auto result = to_chars(buffer, buffer + sizeof buffer, count_interval_type(2, 5));
            CHECK_CASE("to_chars reports a buffer too small", result.ec == errc::value_too_large);
        }

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("brackets in to_chars and from_chars")
    {
        auto chars_round_trip = [](const auto& t, const FormatSpec& spec, string_view expected)
        {
            char buffer[128] {};
            auto written_result = to_chars(buffer, buffer + sizeof buffer, t, spec);
            auto read = t;
            read = {};
            auto read_result = from_chars(buffer, written_result.ptr, read, spec);
            return written_result.ec == errc {} && string_view(buffer, written_result.ptr) == expected && read_result.ec == errc {}
                && read_result.ptr == written_result.ptr && read == t;
        };

        CHECK_CASE("a cartesian vector writes and reads its brackets",
            chars_round_trip(vector_type(-7.25, 0.0, 123.5), {.separator = ", ", .brackets = {"[", "]"}}, "[-7.25, 0, 123.5]"));
        CHECK_CASE("a homogeneous point too",
            chars_round_trip(homogeneous_point_type(1.5, -2.5, 3.0), {.separator = ",", .brackets = {"(", ")"}}, "(1.5,-2.5,3)"));
        CHECK_CASE("a discrete vector too",
            chars_round_trip(discrete_vector_type(-7L, 0L, 123L), {.separator = " ", .brackets = {"[", "]"}}, "[-7 0 123]"));
        CHECK_CASE("a discrete point too",
            chars_round_trip(discrete_point_type(16L, -10L, 4L), {.separator = ", ", .brackets = {"(", ")"}}, "(16, -10, 4)"));
        CHECK_CASE("a discrete box brackets its lower and upper bounds", chars_round_trip(
            discrete_box_type(geometry::DiscretePoint<long, 2>(1L, -2L), math::DiscreteVector<long, 2>(3L, 4L)),
            {.separator = ", ", .brackets = {"[", "]"}}, "[1, -2] .. [4, 2]"));
        CHECK_CASE("brackets of more than one character",
            chars_round_trip(discrete_vector_type(-7L, 0L, 123L), {.separator = ",", .brackets = {"<<", ">>"}}, "<<-7,0,123>>"));
        CHECK_CASE("and the default brackets write none", chars_round_trip(discrete_vector_type(-7L, 0L, 123L), {.separator = ","}, "-7,0,123"));

        {
            const bounds_type box(point_type(1.0, 2.0, 3.0), point_type(4.0, 5.0, 6.0));
            const FormatSpec spec {.separator = ", ", .brackets = {"[", "]"}, .series_delimiter = ".."};
            char buffer[128] {};
            auto written_result = to_chars(buffer, buffer + sizeof buffer, box, spec);
            bounds_type read;
            auto read_result = from_chars(buffer, written_result.ptr, read, spec);
            CHECK_CASE_EQUAL("a bounding box brackets each of its points", string(buffer, written_result.ptr), "[1, 2, 3]..[4, 5, 6]");
            CHECK_CASE("and reads them back", read_result.ec == errc {} && read == box);
        }

        {
            vector_type v;
            const string s = " [1, 2, 3 ]";
            auto result = from_chars(s.data(), s.data() + s.size(), v, {.separator = ", ", .brackets = {"[", "]"}});
            CHECK_CASE("whitespace before the opening and closing brackets is skipped",
                result.ec == errc {} && result.ptr == s.data() + s.size() && v == vector_type(1.0, 2.0, 3.0));
        }

        {
            vector_type v;
            const string unopened = "1, 2, 3]";
            const string unclosed = "[1, 2, 3";
            const FormatSpec spec {.separator = ", ", .brackets = {"[", "]"}};
            CHECK_CASE("a missing opening or closing bracket fails",
                from_chars(unopened.data(), unopened.data() + unopened.size(), v, spec).ec == errc::invalid_argument
                && from_chars(unclosed.data(), unclosed.data() + unclosed.size(), v, spec).ec == errc::invalid_argument);
        }

        {
            char buffer[7] {};
            auto result = to_chars(buffer, buffer + sizeof buffer, discrete_vector_type(1L, 2L, 3L), {.separator = ", ", .brackets = {"[", "]"}});
            CHECK_CASE("to_chars reports a buffer with no room for the closing bracket", result.ec == errc::value_too_large);
        }

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("homogeneous text")
    {
        homogeneous_point_type p(6.0, -10.0, 14.0);
        p.rep()[3] = 2.0;
        homogeneous_vector_type v(1.5, -2.5, 3.0);
        const geometry::Segment<homogeneous_traits_type> segment(p, homogeneous_point_type(1.0, 2.0, 3.0));
        point_type cartesian_point(1.0, 2.0, 3.0);

        CHECK_CASE_EQUAL("cartesian text divides a point by its weight", std::format("{}", p), "3 -5 7");
        CHECK_CASE_EQUAL("homogeneous text writes every stored value", std::format("{:/h}", p), "6 -10 14 2");
        CHECK_CASE_EQUAL("a vector's weight too", std::format("{:/h/s, /o[/c]}", v), "[1.5, -2.5, 3, 0]");
        CHECK_CASE_EQUAL("a cartesian type ignores it", std::format("{:/h}", cartesian_point), "1 2 3");
        CHECK_CASE_EQUAL("a series type passes it to its elements", std::format("{:/h}", segment), "6 -10 14 2, 1 2 3 1");

        {
            ostringstream os;
            os << homogeneous << p << ' ' << cartesian << p;
            CHECK_CASE_EQUAL("the manipulators set a stream's coordinate form", os.str(), "6 -10 14 2 3 -5 7");
        }

        {
            istringstream is("6 -10 14 2");
            homogeneous_point_type read;
            is >> homogeneous >> read;
            CHECK_CASE("a stream reads every stored value", !is.fail() && read.rep() == p.rep());
        }

        {
            char buffer[64] {};
            auto written_result = to_chars(buffer, buffer + sizeof buffer, p, {.homogeneous = true});
            homogeneous_point_type read;
            auto read_result = from_chars(buffer, written_result.ptr, read, {.homogeneous = true});
            CHECK_CASE_EQUAL("to_chars writes every stored value", string(buffer, written_result.ptr), "6 -10 14 2");
            CHECK_CASE("and from_chars reads them back", read_result.ec == errc {} && read.rep() == p.rep());
        }

        CHECK_THROWS_AS((void)std::vformat("{:/hx}", std::make_format_args(p)), std::format_error);
        CHECK_THROWS_AS((void)std::vformat("{:/h/h}", std::make_format_args(p)), std::format_error);

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("separators and series delimiters on streams")
    {
        auto bounds_are = []<typename T>(const optional<math::Interval<T>>& interval, T lower, T upper)
        {
            return interval && interval->lower() == lower && interval->upper() == upper;
        };

        {
            ostringstream os;
            os << separator_delim("\t") << vector_type(-7.25, 0.0, 123.5);
            CHECK_CASE_EQUAL("a stream writes a separator that the unquoted spec cannot hold", os.str(), "-7.25\t0\t123.5");
            CHECK_CASE("and reads it back", read_back(vector_type(-7.25, 0.0, 123.5), "\t") == vector_type(-7.25, 0.0, 123.5));
        }


        CHECK_CASE("a stream reads back a series delimiter with a '|' or a '_'",
            bounds_are(read_back(integer_interval_type(-3, 7), "", "", "", "|"), -3, 7)
            && bounds_are(read_back(integer_interval_type(-3, 7), "", "", "", "_to_"), -3, 7));

        {
            ostringstream os;
            os << integer_interval_type(-3, 7) << ' ' << series_delim("..") << integer_interval_type(-3, 7);
            CHECK_CASE_EQUAL("a stream writes an interval with the default series delimiter or the one series_delim sets", os.str(),
                "-3 .. 7 -3..7");
        }

        CHECK_CASE("an interval reads back what a stream wrote", bounds_are(read_back(integer_interval_type(-3, 7), ""), -3, 7)
            && bounds_are(read_back(integer_interval_type(-3, 7), "", "", "", ".."), -3, 7));
        CHECK_CASE("a real interval reads back with the compact delimiter, its lower bound not taking the delimiter's '.'",
            bounds_are(read_back(real_interval_type(2.0, 5.0), "", "", "", ".."), 2.0, 5.0)
            && bounds_are(read_back(real_interval_type(1.5, 2.25), ""), 1.5, 2.25));
        CHECK_CASE("and with a delimiter that is only whitespace", bounds_are(read_back(count_interval_type(2, 5), "", "", "", " "),
            size_t(2), size_t(5)));

        {
            istringstream is("2..5 9");
            count_interval_type interval;
            int next = 0;
            is >> series_delim("..") >> interval >> next;
            CHECK_CASE("a stream continues after the interval it reads", !is.fail() && interval.lower() == 2 && interval.upper() == 5
                && next == 9);
        }

        {
            istringstream is("2 5");
            count_interval_type interval;
            is >> interval;
            CHECK_CASE("a stream without the series delimiter fails", is.fail());
        }

        {
            ostringstream os;
            os << integer_interval_type(-3, 7);
            CHECK_CASE_EQUAL("a stream with no series delimiter set writes the type's default", os.str(), "-3 .. 7");
            CHECK_CASE("and the series default, for a type without one of its own, is a comma and a space",
                series_delimiter_default == ", " && bounds_delimiter_default == " .. ");
        }

        {
            istringstream is("2, 5");
            count_interval_type interval;
            is >> series_delim(", ") >> interval;
            CHECK_CASE("a series delimiter set on a stream overrides the type's default", !is.fail() && interval.lower() == 2
                && interval.upper() == 5);
        }

        const bounds_type box(point_type(1.0, 2.0, 3.0), point_type(4.0, 5.0, 6.0));

        {
            ostringstream os;
            os << separator_delim(", ") << brackets_delim("[", "]") << series_delim("..") << box;
            CHECK_CASE_EQUAL("a stream writes a bounding box with the separator, brackets, and series delimiter", os.str(),
                "[1, 2, 3]..[4, 5, 6]");
        }

        CHECK_CASE("a bounding box reads back what a stream wrote", read_back(box, "") == box
            && read_back(box, ", ", "[", "]", "..") == box && read_back(box, ",", "(", ")", " to ") == box);

        const geometry::Box<cartesian_traits_type> cell(point_type(1.0, 2.0, 3.0), point_type(4.0, 5.0, 6.0));
        CHECK_CASE_EQUAL("a box formats its bounds as a bounding box does", std::format("{}|{:/n../s, /o[/c]}", cell, cell),
            "1 2 3 .. 4 5 6|[1, 2, 3]..[4, 5, 6]");

        {
            optional<geometry::Box<cartesian_traits_type>> read = read_back(cell, ", ", "[", "]", "..");
            CHECK_CASE("and reads back what a stream wrote", read && read->lower() == cell.lower() && read->upper() == cell.upper());
        }

        {
            char buffer[128] {};
            const FormatSpec spec {.separator = ", ", .brackets = {"(", ")"}, .series_delimiter = ".."};
            auto written_result = to_chars(buffer, buffer + sizeof buffer, cell, spec);
            geometry::Box<cartesian_traits_type> read;
            auto read_result = from_chars(buffer, written_result.ptr, read, spec);
            CHECK_CASE_EQUAL("a box's to_chars writes the separator, brackets, and series delimiter", string(buffer, written_result.ptr),
                "(1, 2, 3)..(4, 5, 6)");
            CHECK_CASE("and from_chars reads them back", read_result.ec == errc {} && read.lower() == cell.lower()
                && read.upper() == cell.upper());
        }

        const discrete_box_type cells(geometry::DiscretePoint<long, 2>(1L, -2L), math::DiscreteVector<long, 2>(3L, 4L));
        CHECK_CASE_EQUAL("a discrete box formats its lower and upper bounds as a bounding box does",
            std::format("{}|{:/n../s, /o[/c]}", cells, cells), "1 -2 .. 4 2|[1, -2]..[4, 2]");
        CHECK_CASE("and reads back what a stream wrote", read_back(cells, "") == cells && read_back(cells, ", ", "[", "]", "..") == cells);

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("locus")
    {
        using geometry::Locus;

        CHECK_CASE_EQUAL("a locus is written by name", std::format("{}|{:>8}", Locus::Segment, Locus::Ray), "Segment|     Ray");
        CHECK_CASE("and its name is read back", geometry::to_locus("Polyhedron") == Locus::Polyhedron && !geometry::to_locus("Cube"));

        {
            char buffer[16] {};
            auto written_result = geometry::to_chars(buffer, buffer + sizeof buffer, Locus::Circle);
            Locus read = Locus::Empty;
            auto read_result = geometry::from_chars(buffer, written_result.ptr, read);
            CHECK_CASE_EQUAL("to_chars writes the name", string(buffer, written_result.ptr), "Circle");
            CHECK_CASE("and from_chars reads it back", read_result.ec == errc {} && read_result.ptr == written_result.ptr && read == Locus::Circle);
        }

        {
            Locus read = Locus::Empty;
            const string s = " Plane, 2";
            auto result = geometry::from_chars(s.data(), s.data() + s.size(), read);
            CHECK_CASE("from_chars reads the letters after any whitespace and stops at the first other character",
                result.ec == errc {} && result.ptr == s.data() + 6 && read == Locus::Plane);
        }

        {
            Locus read = Locus::Point;
            const string s = "Cube";
            auto result = geometry::from_chars(s.data(), s.data() + s.size(), read);
            CHECK_CASE("a name that is not a locus's fails, leaving the locus unchanged", result.ec == errc::invalid_argument && read == Locus::Point);
        }

        {
            char buffer[3] {};
            CHECK_CASE("to_chars reports a buffer too small", geometry::to_chars(buffer, buffer + sizeof buffer, Locus::Sphere).ec == errc::value_too_large);
        }

        {
            istringstream is("Polyline,Surface");
            Locus first = Locus::Empty;
            Locus second = Locus::Empty;
            char comma = '\0';
            is >> first >> comma >> second;
            CHECK_CASE("a stream reads a locus and continues after it", !is.fail() && first == Locus::Polyline && comma == ','
                && second == Locus::Surface);
        }

        {
            istringstream is("Cube");
            Locus read = Locus::Empty;
            is >> read;
            CHECK_CASE("and fails on a name that is not a locus's", is.fail());
        }

        CHECK_CASE("every locus reads back what a stream wrote", std::ranges::all_of(geometry::detail::locus_names, [](string_view name)
        {
            const Locus locus = *geometry::to_locus(name);
            return read_back(locus, "") == locus;
        }));

        return;
    }

    //--------------------------------------------------------------------------
    // Returns true if the value reads back, through a stream and through from_chars, as a value whose formatted text is
    // identical to the original's
    template<typename T>
    bool round_trips(const T& t)
    {
        const optional<T> streamed = read_back(t, "");

        char buffer[512] {};
        auto written_result = to_chars(buffer, buffer + sizeof buffer, t);
        T read;
        auto read_result = from_chars(buffer, written_result.ptr, read);

        return streamed && std::format("{}", *streamed) == std::format("{}", t) && written_result.ec == errc {} && read_result.ec == errc {}
            && read_result.ptr == written_result.ptr && std::format("{}", read) == std::format("{}", t);
    }

    //--------------------------------------------------------------------------
    TEST_CASE("series and composite types")
    {
        using traits_type = cartesian_traits_type;

        const point_type p(1.0, 2.0, 3.0);
        const vector_type x(1.0, 0.0, 0.0);
        const vector_type y(0.0, 1.0, 0.0);
        const vector_type z(0.0, 0.0, 1.0);

        const geometry::Segment<traits_type> segment(p, point_type(4.0, 5.0, 6.0));
        const geometry::Polygon<traits_type> polygon {point_type(0.0, 0.0, 0.0), point_type(1.0, 0.0, 0.0), point_type(1.0, 1.0, 0.0)};
        const geometry::Line<traits_type> line(p, z);
        const geometry::Plane<traits_type> plane(p, z);
        const geometry::Ray<traits_type> ray(p, x);
        const geometry::Sphere<traits_type> sphere(p, 2.5);
        const geometry::Circle<traits_type> circle(p, 2.5);
        const geometry::BoundingSphere<traits_type> bounding_sphere(p, 2.5);
        const geometry::Orientation<traits_type> orientation(z, x);
        const geometry::OrientedPoint<traits_type> oriented_point(p, orientation);
        const geometry::DirectedPoint<traits_type> directed_point(p, y);
        const geometry::DiscreteDirectedPoint<long, 2> discrete_directed_point(geometry::DiscretePoint<long, 2>(1L, -2L),
            math::DiscreteVector<long, 2>(0L, 1L));
        const geometry::HalfPlane<traits_type> halfplane(geometry::Line<traits_type>(p, x), y);
        const geometry::HalfSpace<traits_type> halfspace(plane, z);

        CHECK_CASE_EQUAL("a segment writes its endpoints with the series delimiter", std::format("{}", segment), "1 2 3, 4 5 6");
        CHECK_CASE_EQUAL("a polygon its vertices", std::format("{}", polygon), "0 0 0, 1 0 0, 1 1 0");
        CHECK_CASE_EQUAL("a line its point and direction", std::format("{}", line), "1 2 3, 0 0 1");
        CHECK_CASE_EQUAL("a sphere its center and radius", std::format("{}", sphere), "1 2 3, 2.5");
        CHECK_CASE_EQUAL("an oriented point its position and its orientation's vectors", std::format("{}", oriented_point),
            "1 2 3, 0 0 1, 1 0 0");
        CHECK_CASE_EQUAL("a half plane its line and exterior direction", std::format("{}", halfplane), "1 2 3, 1 0 0, 0 1 0");
        CHECK_CASE_EQUAL("a half space its separator and exterior direction", std::format("{}", halfspace), "1 2 3, 0 0 1, 0 0 1");
        CHECK_CASE_EQUAL("a discrete directed point its position and direction", std::format("{}", discrete_directed_point), "1 -2, 0 1");
        CHECK_CASE_EQUAL("the spec sets every delimiter of a nested type", std::format("{:/n; /s,/o[/c]}", oriented_point),
            "[1,2,3]; [0,0,1]; [1,0,0]");

        CHECK_CASE("every one reads back what it wrote, through a stream and through from_chars", round_trips(segment) && round_trips(polygon)
            && round_trips(line) && round_trips(plane) && round_trips(ray) && round_trips(sphere) && round_trips(circle)
            && round_trips(bounding_sphere) && round_trips(orientation) && round_trips(oriented_point) && round_trips(directed_point)
            && round_trips(discrete_directed_point) && round_trips(halfplane) && round_trips(halfspace));

        {
            optional<geometry::HalfPlane<traits_type>> read = read_back(halfplane, ",", "(", ")", " | ");
            CHECK_CASE("a nested type reads back with the stream's delimiters", read && std::format("{}", *read) == std::format("{}", halfplane));
        }

        {
            geometry::Sphere<traits_type> read;
            const string s = "1 2 2..5";
            auto result = from_chars(s.data(), s.data() + s.size(), read, {.series_delimiter = ".."});
            CHECK_CASE("a component before the series delimiter does not read its '.' as a decimal point",
                result.ec == errc {} && read.center() == point_type(1.0, 2.0, 2.0) && read.radius() == 5.0);
        }

        {
            geometry::Polygon<traits_type> read;
            const string s = "0 0 0, 1 0 0; rest";
            auto result = from_chars(s.data(), s.data() + s.size(), read);
            CHECK_CASE("a polygon reads vertices while a series delimiter follows", result.ec == errc {} && read.vertex_count() == 2
                && result.ptr == s.data() + s.find(';'));
        }

        {
            istringstream is("0 0 0, 1 0 0, 1 1 0 next");
            geometry::Polygon<traits_type> read;
            string next;
            is >> read >> next;
            CHECK_CASE("and so does a stream, continuing after the last vertex", !is.fail() && read.vertex_count() == 3 && next == "next");
        }

        {
            optional<geometry::Polygon<traits_type>> read = read_back(polygon, ",", "(", ")", " | ");
            CHECK_CASE("a stream reads the last bracketed vertex at the end of the text before a series delimiter that begins with a space",
                read && std::format("{}", *read) == std::format("{}", polygon));
        }

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("intersection")
    {
        using traits_type = cartesian_traits_type;

        using intersection_type = geometry::Intersection<traits_type>;

        const point_type p(1.0, 2.0, 3.0);
        const vector_type x(1.0, 0.0, 0.0);
        const vector_type z(0.0, 0.0, 1.0);

        const intersection_type empty;
        const intersection_type at_point(p);
        const intersection_type on_line(geometry::Line<traits_type>(p, z));
        const intersection_type on_ray(geometry::Ray<traits_type>(p, x));
        const intersection_type on_segment(geometry::Segment<traits_type>(p, point_type(4.0, 5.0, 6.0)));
        const intersection_type on_plane(geometry::Plane<traits_type>(p, z));
        const intersection_type on_polygon(geometry::Polygon<traits_type> {point_type(0.0, 0.0, 0.0), point_type(1.0, 0.0, 0.0),
            point_type(1.0, 1.0, 0.0)});

        CHECK_CASE_EQUAL("an intersection writes its locus", std::format("{}", empty), "Empty");
        CHECK_CASE_EQUAL("and its value after the series delimiter", std::format("{}", at_point), "Point, 1 2 3");
        CHECK_CASE_EQUAL("a composite value is an embedded series", std::format("{}", on_segment), "Segment, 1 2 3, 4 5 6");
        CHECK_CASE_EQUAL("to_chars writes what the formatter writes", written(on_polygon), std::format("{}", on_polygon));
        CHECK_CASE("every locus reads back what it wrote, through a stream and through from_chars", round_trips(empty) && round_trips(at_point)
            && round_trips(on_line) && round_trips(on_ray) && round_trips(on_segment) && round_trips(on_plane) && round_trips(on_polygon));

        {
            optional<intersection_type> read = read_back(on_segment, ",", "(", ")", " | ");
            CHECK_CASE("a stream reads back with its own delimiters", read && std::format("{}", *read) == std::format("{}", on_segment));
        }

        {
            intersection_type read;
            const string s = "Circle, 1 2 3, 1";
            CHECK_CASE("from_chars rejects a locus an intersection has no value for",
                from_chars(s.data(), s.data() + s.size(), read).ec == errc::invalid_argument);
        }

        return;
    }

} // namespace
