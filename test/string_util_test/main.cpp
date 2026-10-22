//------------------------------------------------------------------------------
// string_util_test
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/common/string_util.hpp"
#include "quetzal/geometry/DiscretePoint.hpp"
#include "quetzal/geometry/Point.hpp"
#include "quetzal/math/CartesianTraits.hpp"
#include "quetzal/math/DiscreteVector.hpp"
#include "quetzal/math/HomogeneousTraits.hpp"
#include "quetzal/math/Vector.hpp"
#include "quetzal/testing/named_checks.hpp"
#include <doctest/doctest.h>
#include <format>
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
    TEST_CASE("skip_object_delimiter and skip_element_delimiter")
    {
        {
            istringstream is("  ( rest");
            skip_object_delimiter(is, '(');
            string rest;
            is >> rest;
            CHECK_CASE("the object delimiter is consumed past leading whitespace", is.good() || is.eof());
            CHECK_CASE_EQUAL("and what follows it is left", rest, "rest");
        }

        {
            istringstream is("x");
            skip_object_delimiter(is, '(');
            CHECK_CASE("a missing object delimiter sets failbit", is.fail());
        }

        {
            istringstream is(", rest");
            skip_element_delimiter(is, ",");
            string rest;
            is >> rest;
            CHECK_CASE_EQUAL("the element delimiter is consumed", rest, "rest");
        }

        {
            istringstream is("rest");
            skip_element_delimiter(is, "");
            string rest;
            is >> rest;
            CHECK_CASE("an empty pattern skips nothing and succeeds", !is.fail());
            CHECK_CASE_EQUAL("leaving the stream where it was", rest, "rest");
        }

        {
            istringstream is("x");
            skip_element_delimiter(is, ",");
            CHECK_CASE("a missing element delimiter sets failbit", is.fail());
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
    // Reading matches the delimiter between components exactly
    TEST_CASE("element delimiters")
    {
        {
            discrete_vector_type v;
            string s = "16,10,4";
            auto result = math::from_chars(s.data(), s.data() + s.size(), v, ",");
            CHECK_CASE("a named delimiter reads the components it separates",
                result.ec == errc {} && result.ptr == s.data() + s.size() && v == discrete_vector_type(16L, 10L, 4L));
        }

        {
            discrete_vector_type v;
            string s = "16 10 4";
            auto result = math::from_chars(s.data(), s.data() + s.size(), v, ",");
            CHECK_CASE("and rejects a string the delimiter is absent from", result.ec == errc::invalid_argument);
            CHECK_CASE("reporting where it expected one", result.ptr == s.data() + 2);
        }

        {
            char buffer[64];
            discrete_vector_type v(16L, 10L, 4L);
            auto result = math::to_chars(buffer, buffer + sizeof buffer, v, ",");
            CHECK_CASE("writing uses the delimiter given", string(buffer, result.ptr) == "16,10,4");
        }

        {
            char buffer[64];
            discrete_vector_type v(7L, -3L, 0L);
            discrete_vector_type w;
            auto written = math::to_chars(buffer, buffer + sizeof buffer, v, ", ");
            auto read = math::from_chars(buffer, written.ptr, w, ", ");
            CHECK_CASE("a multi-character delimiter round trips",
                read.ec == errc {} && read.ptr == written.ptr && w == v);
        }

        {
            point_type p;
            string s = "1.5,-2.5,3";
            auto result = geometry::from_chars(s.data(), s.data() + s.size(), p, ",");
            CHECK_CASE("a cartesian point takes one too",
                result.ec == errc {} && p.x() == 1.5 && p.y() == -2.5 && p.z() == 3.0);
        }

        {
            istringstream is("16,10,4");
            discrete_vector_type v;
            is >> element_delim(",") >> v;
            CHECK_CASE("a stream reads what the manipulator names",
                !is.fail() && v == discrete_vector_type(16L, 10L, 4L));
        }

        {
            ostringstream os;
            os << element_delim(",") << discrete_vector_type(16L, 10L, 4L);
            CHECK_CASE("and writes the same", os.str() == "16,10,4");
        }

        {
            istringstream is("1,2,3");
            vector_type v;
            is >> element_delim(",") >> v;
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
            auto result = array_to_chars(buffer, buffer + sizeof buffer, discrete_vector_type(1L, 2L, 3L), ", ");
            CHECK_CASE("a delimiter that will not fit is an error", result.ec == errc::value_too_large);
        }

        CHECK_CASE_EQUAL("a delimiter other than a space is written",
            [] { char buffer[32] {}; auto r = array_to_chars(buffer, buffer + sizeof buffer, discrete_vector_type(1L, 2L, 3L), ", ");
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
        CHECK_CASE_EQUAL("to_array_string brackets and comma separates", to_array_string(vector_type(1.0, 2.0, 3.0)), "[1, 2, 3]");
        CHECK_CASE_EQUAL("an optional with a value", to_string(optional<int>(7)), "7");
        CHECK_CASE_EQUAL("an optional without one", to_string(optional<int>()), "nullopt");

        return;
    }

    //--------------------------------------------------------------------------
    // The manipulators and the format spec they produce are what put operator<< and
    // std::format in agreement over an array type
    TEST_CASE("object_delim, element_delim and array_format_spec")
    {
        {
            ostringstream os;
            os << vector_type(1.0, 2.0, 3.0);
            CHECK_CASE_EQUAL("the default is space separated and unbracketed", os.str(), "1 2 3");
        }

        {
            ostringstream os;
            os << object_delim('[') << element_delim(", ") << vector_type(1.0, 2.0, 3.0);
            CHECK_CASE_EQUAL("the manipulators bracket and separate", os.str(), "[1, 2, 3]");
        }

        {
            ostringstream os;
            os << object_delim('[') << element_delim(", ") << object_delim('\0') << vector_type(1.0, 2.0, 3.0);
            CHECK_CASE_EQUAL("a null object delimiter resets the bracket", os.str(), "1, 2, 3");
        }

        {
            ostringstream os;
            os << element_delim(", ") << element_delim("") << vector_type(1.0, 2.0, 3.0);
            CHECK_CASE_EQUAL("an empty element delimiter leaves the one in effect", os.str(), "1, 2, 3");
        }

        {
            ostringstream os;
            os << object_delim('(') << element_delim(", ");
            CHECK_CASE_EQUAL("the spec reflects the manipulators", array_format_spec(os).substr(0, 3), "(,_");
        }

        {
            ostringstream os;
            CHECK_CASE_EQUAL("and its defaults where none are set", array_format_spec(os).substr(0, 1), "_");
        }

        return;
    }

} // namespace
