//------------------------------------------------------------------------------
// expression_test
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/expression/Parser.hpp"
#include "quetzal/testing/named_checks.hpp"
#include <doctest/doctest.h>
#include <exception>
#include <format>
#include <optional>
#include <sstream>
#include <string>

using namespace std;
using namespace quetzal::expression;

namespace
{

    using value_type = float;

    //--------------------------------------------------------------------------
    // Returns the expression as it writes itself, for a failure's detail
    string written(const Expression& expression)
    {
        ostringstream os;
        os << expression;
        return os.str();
    }

    //--------------------------------------------------------------------------
    template<typename T>
    void check_value(Parser& parser, const SymbolTable& symbol_table, const string& text, const T& expected)
    {
        optional<Expression> expression = parser.parse(text);
        if (!expression)
        {
            CHECK_CASE(text, false, "unexpected parse error");
            return;
        }

        T value = expression->evaluate<T>(symbol_table);
        ostringstream os;
        os << "parsed as " << *expression << ", expected " << expected << ", got " << value;
        CHECK_CASE(text, value == expected, os.str());
        return;
    }

    //--------------------------------------------------------------------------
    template<typename T>
    void check_evaluation_fails(Parser& parser, const SymbolTable& symbol_table, const string& text)
    {
        optional<Expression> expression = parser.parse(text);
        if (!expression)
        {
            CHECK_CASE(format("{} fails to evaluate", text), false, "unexpected parse error");
            return;
        }

        bool failed = false;
        ostringstream os;

        try
        {
            os << "parsed as " << *expression << ", evaluated as " << expression->evaluate<T>(symbol_table);
        }
        catch (const exception&)
        {
            failed = true;
        }

        CHECK_CASE(format("{} fails to evaluate", text), failed, os.str());
        return;
    }

    //--------------------------------------------------------------------------
    void check_parse_fails(Parser& parser, const string& text)
    {
        optional<Expression> expression = parser.parse(text);
        CHECK_CASE(format("\"{}\" does not parse", text), !expression.has_value(), expression ? written(*expression) : string());
        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("expressions without symbols")
    {
        Parser parser;
        SymbolTable st;

        check_parse_fails(parser, "");
        check_value<value_type>(parser, st, "0", value_type(0));
        check_value<value_type>(parser, st, "1", value_type(1));
        check_value<value_type>(parser, st, "-1", value_type(-1));
        check_value<value_type>(parser, st, "32 + 16", value_type(48));
        check_value<value_type>(parser, st, "32 - 16", value_type(16));
        check_value<value_type>(parser, st, "32 * 16", value_type(512));
        check_value<value_type>(parser, st, "32 / 16", value_type(2));
        check_value<value_type>(parser, st, "abs(2)", value_type(2));
        check_value<value_type>(parser, st, "abs(-2)", value_type(2));
        check_value<value_type>(parser, st, "min(32, 16)", value_type(16));
        check_value<value_type>(parser, st, "min(16, 32)", value_type(16));
        check_value<value_type>(parser, st, "max(32, 16)", value_type(32));
        check_value<value_type>(parser, st, "max(16, 32)", value_type(32));
        check_parse_fails(parser, "max(32)");
        check_value<value_type>(parser, st, "(0)", value_type(0));
        check_value<value_type>(parser, st, "(1)", value_type(1));
        check_value<value_type>(parser, st, "1 + 2 * 3 + (4 + 5) * 6", value_type(61));
        check_evaluation_fails<value_type>(parser, st, "x");
        check_evaluation_fails<value_type>(parser, st, "x + y");
        check_evaluation_fails<value_type>(parser, st, "x + 2");
        check_evaluation_fails<value_type>(parser, st, "2 + x");

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("expressions with symbols")
    {
        Parser parser;
        SymbolTable st;

        st["x"] = "160";
        st["y"] = "120";
        st["width"] = "640";
        st["height"] = "480";
        st["left"] = "100";
        st["top"] = "80";
        st["right"] = "612";
        st["bottom"] = "336";

        check_parse_fails(parser, "");
        check_value<value_type>(parser, st, "0", value_type(0));
        check_value<value_type>(parser, st, "1", value_type(1));
        check_value<value_type>(parser, st, "-1", value_type(-1));
        check_value<value_type>(parser, st, "32 + 16", value_type(48));
        check_value<value_type>(parser, st, "32 - 16", value_type(16));
        check_value<value_type>(parser, st, "32 * 16", value_type(512));
        check_value<value_type>(parser, st, "32 / 16", value_type(2));
        check_value<value_type>(parser, st, "abs(2)", value_type(2));
        check_value<value_type>(parser, st, "abs(-2)", value_type(2));
        check_value<value_type>(parser, st, "min(32, 16)", value_type(16));
        check_value<value_type>(parser, st, "min(16, 32)", value_type(16));
        check_value<value_type>(parser, st, "max(32, 16)", value_type(32));
        check_value<value_type>(parser, st, "max(16, 32)", value_type(32));
        check_value<value_type>(parser, st, "(0)", value_type(0));
        check_value<value_type>(parser, st, "(1)", value_type(1));
        check_value<value_type>(parser, st, "abs( 32)", value_type(32));
        check_value<value_type>(parser, st, "abs(-32 )", value_type(32));
        check_value<value_type>(parser, st, "-abs( - 32 )", value_type(-32));
        check_value<value_type>(parser, st, "1 + 2 * 3 + (4 + 5) * 6", value_type(61));
        check_evaluation_fails<value_type>(parser, st, "z");
        check_value<value_type>(parser, st, "x", value_type(160));
        check_value<value_type>(parser, st, "x + y", value_type(280));
        check_value<value_type>(parser, st, "x + 2", value_type(162));
        check_value<value_type>(parser, st, "2 + x", value_type(162));
        check_evaluation_fails<value_type>(parser, st, "x + z");
        check_evaluation_fails<value_type>(parser, st, "z + x");
        check_value<value_type>(parser, st, "-x", value_type(-160));
        check_value<value_type>(parser, st, "x + width", value_type(800));
        check_value<value_type>(parser, st, "(y+ height)", value_type(600));
        check_value<value_type>(parser, st, "right-left", value_type(512));
        check_value<value_type>(parser, st, "bottom -top", value_type(256));
        check_value<value_type>(parser, st, "x - - width", value_type(800));
        check_value<value_type>(parser, st, "x -- width", value_type(800));
        check_parse_fails(parser, "x -+ width");
        check_value<value_type>(parser, st, "abs(right - left)", value_type(512));
        check_value<value_type>(parser, st, "abs(left-right)", value_type(512));
        check_parse_fails(parser, "max(left)");
        check_value<value_type>(parser, st, "3*4+2048/abs(-(right-left))*min(4,width)+max(right,bottom)", value_type(640));
        check_value<value_type>(parser, st, "3*4+2048/abs(-(right-left))*min(4,width)", value_type(28));
        check_value<value_type>(parser, st, "2048/abs(-(right-left))*min(4,width)", value_type(16));
        check_value<value_type>(parser, st, "2048/abs(-(right-left))", value_type(4));
        check_value<value_type>(parser, st, "abs(-(right-left))*min(4,width)", value_type(2048));
        check_value<value_type>(parser, st, "3 - 4 + 5", value_type(4));
        check_value<value_type>(parser, st, "16 / 4 * 2", value_type(8));
        check_value<value_type>(parser, st, "mod(7, 3)", value_type(1));
        check_value<value_type>(parser, st, "mod(7.5, 2)", value_type(1.5));

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("integer expressions")
    {
        Parser parser;
        SymbolTable sti;
        sti["x"] = "10";
        sti["y"] = "32";

        check_value<int>(parser, sti, "min(7, 2) + max(3, abs(-9))", 11);
        check_value<int>(parser, sti, "x + y", 42);
        check_value<int>(parser, sti, "16 / 4 * 2", 8);
        check_value<int>(parser, sti, "mod(7, 3)", 1);

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("text expressions")
    {
        Parser parser;
        SymbolTable sts;
        sts["a"] = "hello ";
        sts["b"] = "world";

        check_value<string>(parser, sts, "a + b", string("hello world"));
        check_evaluation_fails<string>(parser, sts, "a - b");

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("one expression evaluated as two types")
    {
        Parser parser;
        Expression shared_expr = *parser.parse("x + y");

        SymbolTable st_shared;
        st_shared["x"] = "1.5";
        st_shared["y"] = "2.5";

        CHECK_CASE("evaluated as float", shared_expr.evaluate<float>(st_shared) == 4.0f);

        // should this truncate or round? ...
        bool failed = false;
        try
        {
            static_cast<void>(shared_expr.evaluate<int>(st_shared));
        }
        catch (const exception&)
        {
            failed = true;
        }

        CHECK_CASE("evaluating 1.5 and 2.5 as int is an error", failed);

        return;
    }

} // namespace
