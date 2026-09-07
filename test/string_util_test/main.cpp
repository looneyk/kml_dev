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
#include <exception>
#include <format>
#include <iostream>
#include <sstream>
#include <string>

using namespace std;
using namespace quetzal;

namespace
{

    static constexpr auto escape_reset_string = "\x1b[0m";
    static constexpr auto escape_rendition_pattern = "\x1b[38;5;{}m";

    size_t g_case_count = 0;
    size_t g_failure_count = 0;

    //--------------------------------------------------------------------------
    void report_group(const string& name)
    {
        cout << name << endl;
        return;
    }

    //--------------------------------------------------------------------------
    // Every case reports its own line so that a failure is identifiable by name
    void report_case(const string& name, bool b, const string& detail = {})
    {
        ++g_case_count;

        if (b)
        {
            cout << format(escape_rendition_pattern, 34) << "    ok   " << escape_reset_string << name << endl;
            return;
        }

        ++g_failure_count;
        cout << format(escape_rendition_pattern, 196) << "    FAIL " << escape_reset_string << name;
        if (!detail.empty())
        {
            cout << ": " << detail;
        }
        cout << endl;
        return;
    }

    //--------------------------------------------------------------------------
    void report_case_equal(const string& name, const string& value, const string& value_expected)
    {
        report_case(name, value == value_expected, format("expected \"{}\", got \"{}\"", value_expected, value));
        return;
    }

    //--------------------------------------------------------------------------
    int report_results()
    {
        if (g_failure_count == 0)
        {
            cout << format(escape_rendition_pattern, 34) << format("OK: {} cases", g_case_count) << escape_reset_string << endl;
            return 0;
        }

        cout << format(escape_rendition_pattern, 196) << format("Error: {} of {} cases failed", g_failure_count, g_case_count) << escape_reset_string << endl;
        return 1;
    }

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
    // What to_chars wrote, or a marker naming the error, so that a case reports the difference
    // rather than only that there was one
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
    void test_skip_whitespace()
    {
        report_group("skip_whitespace");

        string s = "  \t x";
        report_case("spaces and tabs are skipped", skip_whitespace(s.data(), s.data() + s.size()) == s.data() + 4);

        string none = "x  ";
        report_case("a non-blank first position is returned as it is", skip_whitespace(none.data(), none.data() + none.size()) == none.data());

        string blank = "   ";
        report_case("all blank reaches the end", skip_whitespace(blank.data(), blank.data() + blank.size()) == blank.data() + blank.size());

        string empty;
        report_case("an empty range is returned as it is", skip_whitespace(empty.data(), empty.data()) == empty.data());

        // Only space and tab count, so a line break stops the skip
        string newline = " \n ";
        report_case("a line break is not skipped", skip_whitespace(newline.data(), newline.data() + newline.size()) == newline.data() + 1);

        return;
    }

    //--------------------------------------------------------------------------
    void test_skip_delimiters()
    {
        report_group("skip_object_delimiter and skip_element_delimiter");

        {
            istringstream is("  ( rest");
            skip_object_delimiter(is, '(');
            string rest;
            is >> rest;
            report_case("the object delimiter is consumed past leading whitespace", is.good() || is.eof());
            report_case_equal("and what follows it is left", rest, "rest");
        }

        {
            istringstream is("x");
            skip_object_delimiter(is, '(');
            report_case("a missing object delimiter sets failbit", is.fail());
        }

        {
            istringstream is(", rest");
            skip_element_delimiter(is, ",");
            string rest;
            is >> rest;
            report_case_equal("the element delimiter is consumed", rest, "rest");
        }

        {
            istringstream is("rest");
            skip_element_delimiter(is, "");
            string rest;
            is >> rest;
            report_case("an empty pattern skips nothing and succeeds", !is.fail());
            report_case_equal("leaving the stream where it was", rest, "rest");
        }

        {
            istringstream is("x");
            skip_element_delimiter(is, ",");
            report_case("a missing element delimiter sets failbit", is.fail());
        }

        return;
    }

    //--------------------------------------------------------------------------
    // The one pair of conversions all four vector and point types go through
    void test_array_from_chars()
    {
        report_group("array_from_chars");

        {
            discrete_vector_type v;
            string s = "1 -2 30";
            auto result = from_chars(s.data(), s.data() + s.size(), v);
            report_case("a discrete vector reads every component", result.ec == errc {} && result.ptr == s.data() + s.size());
            report_case("with the values the string names", v == discrete_vector_type(1L, -2L, 30L));
        }

        {
            discrete_vector_type v;
            string s = "  4\t 5   6";
            auto result = from_chars(s.data(), s.data() + s.size(), v);
            report_case("leading and inner whitespace is skipped", result.ec == errc {} && v == discrete_vector_type(4L, 5L, 6L));
        }

        {
            discrete_vector_type v;
            string s = "7 x 9";
            auto result = from_chars(s.data(), s.data() + s.size(), v);
            report_case("a component that cannot be read is an error", result.ec == errc::invalid_argument);
            report_case("reported where it stopped", result.ptr == s.data() + 2);
        }

        {
            discrete_vector_type v;
            string s = "1 2";
            auto result = from_chars(s.data(), s.data() + s.size(), v);
            report_case("a string of too few components is an error", result.ec != errc {});
        }

        {
            discrete_point_type p;
            string s = " 11 -22\t33";
            auto result = from_chars(s.data(), s.data() + s.size(), p);
            report_case("a discrete point reads every component",
                result.ec == errc {} && p.x() == 11L && p.y() == -22L && p.z() == 33L);
        }

        {
            vector_type v;
            string s = "1.5 -2.5 3";
            auto result = from_chars(s.data(), s.data() + s.size(), v);
            report_case("a cartesian vector reads every component",
                result.ec == errc {} && v.x() == 1.5 && v.y() == -2.5 && v.z() == 3.0);
        }

        {
            point_type p;
            string s = "1.5 -2.5 3";
            auto result = from_chars(s.data(), s.data() + s.size(), p);
            report_case("a cartesian point reads every component",
                result.ec == errc {} && p.x() == 1.5 && p.y() == -2.5 && p.z() == 3.0);
        }

        // Homogeneous coordinates are dimension components on the wire; the weight is the
        // caller's to supply, and is what tells a direction from a position
        {
            homogeneous_vector_type v;
            string s = "1.5 -2.5 3";
            auto result = from_chars(s.data(), s.data() + s.size(), v);
            report_case("a homogeneous vector reads three and concludes a weight of 0",
                result.ec == errc {} && result.ptr == s.data() + s.size() && v.weight() == 0.0);
        }

        {
            homogeneous_point_type p;
            string s = "1.5 -2.5 3";
            auto result = from_chars(s.data(), s.data() + s.size(), p);
            report_case("a homogeneous point reads three and concludes a weight of 1",
                result.ec == errc {} && result.ptr == s.data() + s.size() && p.weight() == 1.0);
        }

        return;
    }

    //--------------------------------------------------------------------------
    void test_array_to_chars()
    {
        report_group("array_to_chars");

        report_case_equal("a discrete vector writes its components", written(discrete_vector_type(1L, -2L, 30L)), "1 -2 30");
        report_case_equal("a discrete point writes its components", written(discrete_point_type(11L, -22L, 33L)), "11 -22 33");
        report_case_equal("a cartesian vector writes its components", written(vector_type(1.5, -2.5, 3.0)), "1.5 -2.5 3");
        report_case_equal("a cartesian point writes its components", written(point_type(1.5, -2.5, 3.0)), "1.5 -2.5 3");
        report_case_equal("a homogeneous vector writes the dimension, not the weight", written(homogeneous_vector_type(1.5, -2.5, 3.0)), "1.5 -2.5 3");
        report_case_equal("a homogeneous point writes the dimension, not the weight", written(homogeneous_point_type(1.5, -2.5, 3.0)), "1.5 -2.5 3");

        // A weighted point is reduced before it is written, so what comes out is where it is
        {
            homogeneous_point_type p(6.0, -10.0, 14.0);
            p.rep()[3] = 2.0;
            report_case_equal("a weighted homogeneous point is reduced before writing", written(p), "3 -5 7");
        }

        report_case_equal("a negative zero is written as zero", written(vector_type(-0.0, 1.0, 2.0)), "0 1 2");

        {
            char buffer[4] {};
            auto result = to_chars(buffer, buffer + sizeof buffer, discrete_vector_type(100L, 200L, 300L));
            report_case("a buffer too small for a discrete vector is an error", result.ec == errc::value_too_large);
        }

        {
            char buffer[4] {};
            auto result = to_chars(buffer, buffer + sizeof buffer, vector_type(100.0, 200.0, 300.0));
            report_case("a buffer too small for a cartesian vector is an error", result.ec == errc::value_too_large);
        }

        // The delimiter is not written where there is no room left for it
        {
            char buffer[3] {};
            auto result = array_to_chars(buffer, buffer + sizeof buffer, discrete_vector_type(1L, 2L, 3L), ", ");
            report_case("a delimiter that will not fit is an error", result.ec == errc::value_too_large);
        }

        report_case_equal("a delimiter other than a space is written",
            [] { char buffer[32] {}; auto r = array_to_chars(buffer, buffer + sizeof buffer, discrete_vector_type(1L, 2L, 3L), ", ");
                return r.ec == errc {} ? string(buffer, r.ptr) : string("<error>"); }(), "1, 2, 3");

        return;
    }

    //--------------------------------------------------------------------------
    void test_round_trip()
    {
        report_group("round trip");

        {
            char buffer[64] {};
            auto written_result = to_chars(buffer, buffer + sizeof buffer, discrete_vector_type(-7L, 0L, 123L));
            discrete_vector_type v;
            auto read_result = from_chars(buffer, written_result.ptr, v);
            report_case("a discrete vector survives a round trip",
                read_result.ec == errc {} && v == discrete_vector_type(-7L, 0L, 123L));
        }

        {
            char buffer[64] {};
            auto written_result = to_chars(buffer, buffer + sizeof buffer, discrete_point_type(-7L, 0L, 123L));
            discrete_point_type p;
            auto read_result = from_chars(buffer, written_result.ptr, p);
            report_case("a discrete point survives a round trip",
                read_result.ec == errc {} && p.x() == -7L && p.y() == 0L && p.z() == 123L);
        }

        {
            char buffer[64] {};
            auto written_result = to_chars(buffer, buffer + sizeof buffer, vector_type(-7.25, 0.0, 123.5));
            vector_type v;
            auto read_result = from_chars(buffer, written_result.ptr, v);
            report_case("a cartesian vector survives a round trip",
                read_result.ec == errc {} && v.x() == -7.25 && v.y() == 0.0 && v.z() == 123.5);
        }

        {
            char buffer[64] {};
            auto written_result = to_chars(buffer, buffer + sizeof buffer, homogeneous_point_type(-7.25, 0.0, 123.5));
            homogeneous_point_type p;
            auto read_result = from_chars(buffer, written_result.ptr, p);
            report_case("a homogeneous point survives a round trip with its weight restored",
                read_result.ec == errc {} && p.x() == -7.25 && p.z() == 123.5 && p.weight() == 1.0);
        }

        return;
    }

    //--------------------------------------------------------------------------
    void test_to_type()
    {
        report_group("to_type and to_bool");

        report_case("an integer is read", to_type<int>("42") == 42);
        report_case("a negative integer is read", to_type<long>("-17") == -17);
        report_case("a real is read", to_type<double>("2.5") == 2.5);
        report_case_equal("a string is returned as it is", to_type<string>("text"), "text");
        report_case("true is read", to_type<bool>("true"));
        report_case("1 is read as true", to_bool("1"));
        report_case("false is read", !to_bool("false"));
        report_case("0 is read as false", !to_bool("0"));

        {
            bool threw = false;
            try
            {
                to_bool("yes");
            }
            catch (const domain_error&)
            {
                threw = true;
            }
            report_case("a value that is neither is rejected", threw);
        }

        report_case("try_type reads what it can", try_type<int>("42") == 42);
        report_case("try_type reports what it cannot", !try_type<int>("x").has_value());

        return;
    }

    //--------------------------------------------------------------------------
    void test_to_string()
    {
        report_group("to_string");

        report_case_equal("a string is returned as it is", to_string(string("text")), "text");
        report_case_equal("a wide string is converted", to_string(wstring(L"text")), "text");
        report_case_equal("true", to_string(true), "true");
        report_case_equal("false", to_string(false), "false");
        report_case_equal("an array type is space separated", to_string(vector_type(1.0, 2.0, 3.0)), "1 2 3");
        report_case_equal("to_array_string brackets and comma separates", to_array_string(vector_type(1.0, 2.0, 3.0)), "[1, 2, 3]");
        report_case_equal("an optional with a value", to_string(optional<int>(7)), "7");
        report_case_equal("an optional without one", to_string(optional<int>()), "nullopt");

        return;
    }

    //--------------------------------------------------------------------------
    // The manipulators and the format spec they produce are what put operator<< and
    // std::format in agreement over an array type
    void test_stream_delimiters()
    {
        report_group("object_delim, element_delim and array_format_spec");

        {
            ostringstream os;
            os << vector_type(1.0, 2.0, 3.0);
            report_case_equal("the default is space separated and unbracketed", os.str(), "1 2 3");
        }

        {
            ostringstream os;
            os << object_delim('[') << element_delim(", ") << vector_type(1.0, 2.0, 3.0);
            report_case_equal("the manipulators bracket and separate", os.str(), "[1, 2, 3]");
        }

        {
            ostringstream os;
            os << object_delim('[') << element_delim(", ") << object_delim('\0') << vector_type(1.0, 2.0, 3.0);
            report_case_equal("a null object delimiter resets the bracket", os.str(), "1, 2, 3");
        }

        {
            ostringstream os;
            os << element_delim(", ") << element_delim("") << vector_type(1.0, 2.0, 3.0);
            report_case_equal("an empty element delimiter leaves the one in effect", os.str(), "1, 2, 3");
        }

        {
            ostringstream os;
            os << object_delim('(') << element_delim(", ");
            report_case_equal("the spec reflects the manipulators", array_format_spec(os).substr(0, 3), "(,_");
        }

        {
            ostringstream os;
            report_case_equal("and its defaults where none are set", array_format_spec(os).substr(0, 1), "_");
        }

        return;
    }

} // namespace

//------------------------------------------------------------------------------
int main()
{
    try
    {
        test_skip_whitespace();
        test_skip_delimiters();
        test_array_from_chars();
        test_array_to_chars();
        test_round_trip();
        test_to_type();
        test_to_string();
        test_stream_delimiters();
    }
    catch (const exception& e)
    {
        cout << format(escape_rendition_pattern, 196) << "Error: " << escape_reset_string << e.what() << endl;
        return 1;
    }

    return report_results();
}
