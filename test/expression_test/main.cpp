//------------------------------------------------------------------------------
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/expression/Parser.hpp"
#include <iostream>

using namespace std;
using namespace quetzal::expression;

//------------------------------------------------------------------------------
void test(Parser& parser, const SymbolTable& st, const string& str, float answer)
{
    Expression::pointer_type pe = parser.parse(str);

    cout << str << endl;
    cout << *pe << endl;
    float x = pe->evaluate_float(st);
    cout << x << endl;
    if (x != answer)
    {
        cout << "failed: should be " << answer << endl;
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

    test(parser, st, "", value_type(0));
    test(parser, st, "0", value_type(0));
    test(parser, st, "1", value_type(1));
    test(parser, st, "-1", value_type(-1));
    test(parser, st, "32 + 16", value_type(48));
    test(parser, st, "32 - 16", value_type(16));
    test(parser, st, "32 * 16", value_type(512));
    test(parser, st, "32 / 16", value_type(2));
    test(parser, st, "abs(2)", value_type(2));
    test(parser, st, "abs(-2)", value_type(2));
    test(parser, st, "min(32, 16)", value_type(16));
    test(parser, st, "min(16, 32)", value_type(16));
    test(parser, st, "max(32, 16)", value_type(32));
    test(parser, st, "max(16, 32)", value_type(32));
    test(parser, st, "max(32)", value_type(0));
    test(parser, st, "(0)", value_type(0));
    test(parser, st, "(1)", value_type(1));
    test(parser, st, "1 + 2 * 3 + (4 + 5) * 6", value_type(61));
    test(parser, st, "x", value_type(0));
    test(parser, st, "x + y", value_type(0));
    test(parser, st, "x + 2", value_type(0));
    test(parser, st, "2 + x", value_type(0));

    st["x"] = "160";
    st["y"] = "120";
    st["width"] = "640";
    st["height"] = "480";
    st["left"] = "100";
    st["top"] = "80";
    st["right"] = "612";
    st["bottom"] = "336";

    test(parser, st, "", value_type(0));
    test(parser, st, "0", value_type(0));
    test(parser, st, "1", value_type(1));
    test(parser, st, "-1", value_type(-1));
    test(parser, st, "32 + 16", value_type(48));
    test(parser, st, "32 - 16", value_type(16));
    test(parser, st, "32 * 16", value_type(512));
    test(parser, st, "32 / 16", value_type(2));
    test(parser, st, "abs(2)", value_type(2));
    test(parser, st, "abs(-2)", value_type(2));
    test(parser, st, "min(32, 16)", value_type(16));
    test(parser, st, "min(16, 32)", value_type(16));
    test(parser, st, "max(32, 16)", value_type(32));
    test(parser, st, "max(16, 32)", value_type(32));
    test(parser, st, "(0)", value_type(0));
    test(parser, st, "(1)", value_type(1));
    test(parser, st, "abs( 32)", value_type(32));
    test(parser, st, "abs(-32 )", value_type(32));
    test(parser, st, "-abs( - 32 )", value_type(-32));
    test(parser, st, "1 + 2 * 3 + (4 + 5) * 6", value_type(61));
    test(parser, st, "z", value_type(0));
    test(parser, st, "x", value_type(160));
    test(parser, st, "x + y", value_type(280));
    test(parser, st, "x + 2", value_type(162));
    test(parser, st, "2 + x", value_type(162));
    test(parser, st, "x + z", value_type(0));
    test(parser, st, "z + x", value_type(0));
    test(parser, st, "-x", value_type(-160));
    test(parser, st, "x + width", value_type(800));
    test(parser, st, "(y+ height)", value_type(600));
    test(parser, st, "right-left", value_type(512));
    test(parser, st, "bottom -top", value_type(256));
    test(parser, st, "x - - width", value_type(800));
    test(parser, st, "x -- width", value_type(800));
    test(parser, st, "x -+ width", value_type(0));
    test(parser, st, "abs(right - left)", value_type(512));
    test(parser, st, "abs(left-right)", value_type(512));
    test(parser, st, "max(left)", value_type(0));
    test(parser, st, "3*4+2048/abs(-(right-left))*min(4,width)+max(right,bottom)", value_type(640));
    test(parser, st, "3*4+2048/abs(-(right-left))*min(4,width)", value_type(28));
    test(parser, st, "2048/abs(-(right-left))*min(4,width)", value_type(16));
    test(parser, st, "2048/abs(-(right-left))", value_type(4));
    test(parser, st, "abs(-(right-left))*min(4,width)", value_type(2048));
    test(parser, st, "3 - 4 + 5", value_type(4));
    test(parser, st, "16 / 4 * 2", value_type(8));

    cout << "Done" << endl;

    char c;
    cin >> c;

    return 0;
}
