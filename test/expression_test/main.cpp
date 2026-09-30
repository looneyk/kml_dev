//------------------------------------------------------------------------------
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/expression/Parser.hpp"
#include <iostream>

using namespace std;
using namespace quetzal::expression;

//------------------------------------------------------------------------------
template<typename T>
void test(Parser& parser, const SymbolTable& st, const string& str, T expected)
{
    optional<Expression> oexpr = parser.parse(str);

    cout << str << endl;

    if (!oexpr)
    {
        cout << "failed: unexpected parse error" << endl;
        return;
    }

    cout << *oexpr << endl;
    T x = oexpr->evaluate<T>(st);
    cout << x << endl;
    if (x != expected)
    {
        cout << "failed: should be " << expected << endl;
    }

    return;
}

//------------------------------------------------------------------------------
template<typename T>
void test_error(Parser& parser, const SymbolTable& st, const string& str)
{
    optional<Expression> oexpr = parser.parse(str);

    cout << str << endl;

    if (!oexpr)
    {
        cout << "failed: unexpected parse error" << endl;
        return;
    }

    cout << *oexpr << endl;

    try
    {
        T x = oexpr->evaluate<T>(st);
        cout << "failed: expected an error, got " << x << endl;
    }
    catch (const exception& e)
    {
        cout << e.what() << endl;
    }

    return;
}

//------------------------------------------------------------------------------
void test_parse_fails(Parser& parser, const string& str)
{
    optional<Expression> oexpr = parser.parse(str);

    cout << str << endl;

    if (oexpr)
    {
        cout << "failed: expected a parse error, got " << *oexpr << endl;
    }
    else
    {
        cout << "(parse error, as expected)" << endl;
    }

    return;
}

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    argc;
    argv;

    typedef float value_type;

    Parser parser;

    SymbolTable st;

    test_parse_fails(parser, "");
    test<value_type>(parser, st, "0", value_type(0));
    test<value_type>(parser, st, "1", value_type(1));
    test<value_type>(parser, st, "-1", value_type(-1));
    test<value_type>(parser, st, "32 + 16", value_type(48));
    test<value_type>(parser, st, "32 - 16", value_type(16));
    test<value_type>(parser, st, "32 * 16", value_type(512));
    test<value_type>(parser, st, "32 / 16", value_type(2));
    test<value_type>(parser, st, "abs(2)", value_type(2));
    test<value_type>(parser, st, "abs(-2)", value_type(2));
    test<value_type>(parser, st, "min(32, 16)", value_type(16));
    test<value_type>(parser, st, "min(16, 32)", value_type(16));
    test<value_type>(parser, st, "max(32, 16)", value_type(32));
    test<value_type>(parser, st, "max(16, 32)", value_type(32));
    test_parse_fails(parser, "max(32)");
    test<value_type>(parser, st, "(0)", value_type(0));
    test<value_type>(parser, st, "(1)", value_type(1));
    test<value_type>(parser, st, "1 + 2 * 3 + (4 + 5) * 6", value_type(61));
    test_error<value_type>(parser, st, "x");
    test_error<value_type>(parser, st, "x + y");
    test_error<value_type>(parser, st, "x + 2");
    test_error<value_type>(parser, st, "2 + x");

    st["x"] = "160";
    st["y"] = "120";
    st["width"] = "640";
    st["height"] = "480";
    st["left"] = "100";
    st["top"] = "80";
    st["right"] = "612";
    st["bottom"] = "336";

    test_parse_fails(parser, "");
    test<value_type>(parser, st, "0", value_type(0));
    test<value_type>(parser, st, "1", value_type(1));
    test<value_type>(parser, st, "-1", value_type(-1));
    test<value_type>(parser, st, "32 + 16", value_type(48));
    test<value_type>(parser, st, "32 - 16", value_type(16));
    test<value_type>(parser, st, "32 * 16", value_type(512));
    test<value_type>(parser, st, "32 / 16", value_type(2));
    test<value_type>(parser, st, "abs(2)", value_type(2));
    test<value_type>(parser, st, "abs(-2)", value_type(2));
    test<value_type>(parser, st, "min(32, 16)", value_type(16));
    test<value_type>(parser, st, "min(16, 32)", value_type(16));
    test<value_type>(parser, st, "max(32, 16)", value_type(32));
    test<value_type>(parser, st, "max(16, 32)", value_type(32));
    test<value_type>(parser, st, "(0)", value_type(0));
    test<value_type>(parser, st, "(1)", value_type(1));
    test<value_type>(parser, st, "abs( 32)", value_type(32));
    test<value_type>(parser, st, "abs(-32 )", value_type(32));
    test<value_type>(parser, st, "-abs( - 32 )", value_type(-32));
    test<value_type>(parser, st, "1 + 2 * 3 + (4 + 5) * 6", value_type(61));
    test_error<value_type>(parser, st, "z");
    test<value_type>(parser, st, "x", value_type(160));
    test<value_type>(parser, st, "x + y", value_type(280));
    test<value_type>(parser, st, "x + 2", value_type(162));
    test<value_type>(parser, st, "2 + x", value_type(162));
    test_error<value_type>(parser, st, "x + z");
    test_error<value_type>(parser, st, "z + x");
    test<value_type>(parser, st, "-x", value_type(-160));
    test<value_type>(parser, st, "x + width", value_type(800));
    test<value_type>(parser, st, "(y+ height)", value_type(600));
    test<value_type>(parser, st, "right-left", value_type(512));
    test<value_type>(parser, st, "bottom -top", value_type(256));
    test<value_type>(parser, st, "x - - width", value_type(800));
    test<value_type>(parser, st, "x -- width", value_type(800));
    test_parse_fails(parser, "x -+ width");
    test<value_type>(parser, st, "abs(right - left)", value_type(512));
    test<value_type>(parser, st, "abs(left-right)", value_type(512));
    test_parse_fails(parser, "max(left)");
    test<value_type>(parser, st, "3*4+2048/abs(-(right-left))*min(4,width)+max(right,bottom)", value_type(640));
    test<value_type>(parser, st, "3*4+2048/abs(-(right-left))*min(4,width)", value_type(28));
    test<value_type>(parser, st, "2048/abs(-(right-left))*min(4,width)", value_type(16));
    test<value_type>(parser, st, "2048/abs(-(right-left))", value_type(4));
    test<value_type>(parser, st, "abs(-(right-left))*min(4,width)", value_type(2048));
    test<value_type>(parser, st, "3 - 4 + 5", value_type(4));
    test<value_type>(parser, st, "16 / 4 * 2", value_type(8));
    test<value_type>(parser, st, "mod(7, 3)", value_type(1));
    test<value_type>(parser, st, "mod(7.5, 2)", value_type(1.5));

    SymbolTable sti;
    sti["x"] = "10";
    sti["y"] = "32";

    test<int>(parser, sti, "min(7, 2) + max(3, abs(-9))", 11);
    test<int>(parser, sti, "x + y", 42);
    test<int>(parser, sti, "16 / 4 * 2", 8);
    test<int>(parser, sti, "mod(7, 3)", 1);

    SymbolTable sts;
    sts["a"] = "hello ";
    sts["b"] = "world";

    test<string>(parser, sts, "a + b", string("hello world"));
    test_error<string>(parser, sts, "a - b");

    Expression shared_expr = *parser.parse("x + y");

    SymbolTable st_shared;
    st_shared["x"] = "1.5";
    st_shared["y"] = "2.5";

    cout << "shared expression and symbol table, parsed once:" << endl;
    cout << shared_expr << endl;
    cout << "evaluated as float: " << shared_expr.evaluate<float>(st_shared) << endl;

    // should this truncate or round? ...
    try
    {
        int x = shared_expr.evaluate<int>(st_shared);
        cout << "failed: expected an error evaluating 1.5 and 2.5 as int, got " << x << endl;
    }
    catch (const exception& e)
    {
        cout << "evaluated as int: " << e.what() << endl;
    }

    cout << "Done" << endl;

    return 0;
}
