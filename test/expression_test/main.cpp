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
        Parser::result_type expression = parser.parse(text, symbol_table);
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
        Parser::result_type expression = parser.parse(text, symbol_table);
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
    void check_parse_fails(Parser& parser, const string& text, const SymbolTable& symbol_table = {})
    {
        Parser::result_type expression = parser.parse(text, symbol_table);
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
        check_parse_fails(parser, "x");
        check_parse_fails(parser, "x + y");
        check_parse_fails(parser, "x + 2");
        check_parse_fails(parser, "2 + x");

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("expressions with symbols")
    {
        Parser parser;
        SymbolTable st;

        st["x"] = 160;
        st["y"] = 120;
        st["width"] = 640;
        st["height"] = 480;
        st["left"] = 100;
        st["top"] = 80;
        st["right"] = 612;
        st["bottom"] = 336;

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
        check_parse_fails(parser, "z", st);
        check_value<value_type>(parser, st, "x", value_type(160));
        check_value<value_type>(parser, st, "x + y", value_type(280));
        check_value<value_type>(parser, st, "x + 2", value_type(162));
        check_value<value_type>(parser, st, "2 + x", value_type(162));
        check_parse_fails(parser, "x + z", st);
        check_parse_fails(parser, "z + x", st);
        check_value<value_type>(parser, st, "-x", value_type(-160));
        check_value<value_type>(parser, st, "x + width", value_type(800));
        check_value<value_type>(parser, st, "(y+ height)", value_type(600));
        check_value<value_type>(parser, st, "right-left", value_type(512));
        check_value<value_type>(parser, st, "bottom -top", value_type(256));
        check_value<value_type>(parser, st, "x - - width", value_type(800));
        check_value<value_type>(parser, st, "x -- width", value_type(800));
        check_parse_fails(parser, "x -+ width", st);
        check_value<value_type>(parser, st, "abs(right - left)", value_type(512));
        check_value<value_type>(parser, st, "abs(left-right)", value_type(512));
        check_parse_fails(parser, "max(left)", st);
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
        sti["x"] = 10;
        sti["y"] = 32;

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
        check_parse_fails(parser, "a - b", sts);

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("string literals")
    {
        Parser parser;
        SymbolTable sts;
        sts["a"] = "hello";
        sts["b"] = "world";

        check_value<string>(parser, sts, R"("text")", string("text"));
        check_value<string>(parser, sts, R"("")", string());
        check_value<string>(parser, sts, R"(a + "_" + b)", string("hello_world"));
        check_value<string>(parser, sts, R"("say \"hi\"")", string(R"(say "hi")"));
        check_value<string>(parser, sts, R"("a\\b")", string(R"(a\b)"));
        check_value<string>(parser, sts, R"(("1" + "2"))", string("12"));
        check_value<string>(parser, sts, R"('text')", string("text"));
        check_value<string>(parser, sts, R"('say "hi"')", string(R"(say "hi")"));
        check_value<string>(parser, sts, R"("it's")", string("it's"));
        check_value<string>(parser, sts, R"('it\'s')", string("it's"));
        check_value<string>(parser, sts, R"('a' + "b")", string("ab"));
        check_parse_fails(parser, R"("unterminated)");
        check_parse_fails(parser, R"("ends in an escape\")");
        check_parse_fails(parser, R"('mismatched")");
        check_evaluation_fails<value_type>(parser, sts, R"("1")");
        check_parse_fails(parser, R"("a" - "b")", sts);

        for (const string& text : {R"("text")", R"(a + "_" + b)", R"("say \"hi\"")", R"("a\\b")", R"(("1" + "2"))", R"('say "hi"')",
            R"('it\'s')", R"('a' + "b")"})
        {
            Parser::result_type expression = parser.parse(text, sts);
            string printed = expression ? written(*expression) : string();
            CHECK_CASE(format("{} prints as written", text), printed == text, printed);
        }

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("node types")
    {
        Parser parser;

        const SymbolTable symbols{{"count", 3}, {"name", "zone"}};
        auto type_of = [&](const string& text) { return parser.parse(text, symbols).value().node().type; };

        CHECK_CASE("a number literal is a number", type_of("12") == DataType::Number);
        CHECK_CASE("a string literal is a string", type_of("'12'") == DataType::String);
        CHECK_CASE("a sum of numbers is a number", type_of("1 + 2") == DataType::Number);
        CHECK_CASE("a sum of strings is a string", type_of("'a' + 'b'") == DataType::String);
        CHECK_CASE("a difference is a number", type_of("2 - 1") == DataType::Number);
        CHECK_CASE("a number symbol is a number", type_of("count") == DataType::Number);
        CHECK_CASE("a string symbol is a string", type_of("name") == DataType::String);
        check_parse_fails(parser, "1 + 'a'", symbols);
        check_parse_fails(parser, "'a' + 1", symbols);
        check_parse_fails(parser, "count + name", symbols);
        CHECK_CASE("a call has its function's return type", type_of("min(count, 2)") == DataType::Number);
        check_parse_fails(parser, "abs('a')", symbols);
        check_parse_fails(parser, "max(count, name)", symbols);
        check_parse_fails(parser, "count - name", symbols);
        check_parse_fails(parser, "name * 2", symbols);
        check_parse_fails(parser, "2 / name", symbols);
        check_parse_fails(parser, "-name", symbols);
        check_parse_fails(parser, "str(count) * 2", symbols);
        CHECK_CASE("a product is a number", type_of("count * 2") == DataType::Number);
        CHECK_CASE("a negation is a number", type_of("-count") == DataType::Number);

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("symbol types")
    {
        Parser parser;
        const SymbolTable symbols{{"count", 3}, {"ratio", 0.5}, {"name", "zone"}, {"digits", "12"}};

        check_value<int>(parser, symbols, "count + 1", 4);
        check_value<double>(parser, symbols, "ratio * 4", 2.0);
        check_value<string>(parser, symbols, "name + '_'", string("zone_"));
        check_value<string>(parser, symbols, "digits", string("12"));
        check_evaluation_fails<int>(parser, symbols, "digits");
        check_evaluation_fails<int>(parser, symbols, "name");

        const SymbolTable qualified{{"region.difficulty", 4}, {"region.name", "crypt"}};
        check_value<int>(parser, qualified, "region.difficulty - 1", 3);
        check_value<string>(parser, qualified, "region.name + '_'", string("crypt_"));
        check_parse_fails(parser, "region.size", qualified);

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("explicit conversion")
    {
        Parser parser;
        const SymbolTable symbols{{"count", 3}, {"ratio", 0.5}, {"name", "zone"}};

        CHECK_CASE("str is a string", parser.parse("str(count)", symbols).value().node().type == DataType::String);
        check_value<string>(parser, symbols, "str(count)", string("3"));
        check_value<string>(parser, symbols, "str(ratio)", string("0.5"));
        check_value<string>(parser, symbols, "str(1 + 2) + 'a'", string("3a"));
        check_value<string>(parser, symbols, "'a' + str(1) + str(2)", string("a12"));
        check_value<string>(parser, symbols, "name + '_' + str(count * 2)", string("zone_6"));
        check_parse_fails(parser, "str(name)", symbols);
        check_evaluation_fails<int>(parser, symbols, "str(count)");

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("evaluation by type")
    {
        Parser parser;
        const SymbolTable symbols{{"count", 3}, {"name", "zone"}};

        check_evaluation_fails<string>(parser, symbols, "1 + 2");
        check_evaluation_fails<string>(parser, symbols, "count");
        check_evaluation_fails<int>(parser, symbols, "'a' + name");

        const Expression expression = parser.parse("count + 1", symbols).value();
        CHECK_THROWS_AS(static_cast<void>(expression.evaluate<int>({{"count", "3"}})), exception);

        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("null")
    {
        Parser parser;
        const SymbolTable symbols{{"count", 3}};

        const Parser::result_type null_expression = parser.parse("null", symbols);
        CHECK_CASE("null parses to an expression of type null", null_expression.has_value()
            && null_expression->node().type == DataType::Null);

        check_parse_fails(parser, "null + 1", symbols);
        check_parse_fails(parser, "'a' + null", symbols);
        check_parse_fails(parser, "null + null", symbols);
        check_parse_fails(parser, "null * 2", symbols);
        check_parse_fails(parser, "-null", symbols);
        check_parse_fails(parser, "abs(null)", symbols);
        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("parse errors")
    {
        Parser parser;
        const SymbolTable symbols{{"count", 3}, {"name", "zone"}};

        auto code_of = [&](const string& text)
        {
            const Parser::result_type parsed = parser.parse(text, symbols);
            return parsed.has_value() ? optional<ParseError::Code>() : parsed.error().code;
        };

        CHECK_CASE("an invalid character", code_of("count # 2") == ParseError::Code::InvalidCharacter);
        CHECK_CASE("an unterminated string", code_of("'zone") == ParseError::Code::UnterminatedString);
        CHECK_CASE("an unexpected token", code_of("count +") == ParseError::Code::UnexpectedToken);
        CHECK_CASE("an unknown identifier", code_of("level") == ParseError::Code::UnknownIdentifier);
        CHECK_CASE("the wrong number of arguments", code_of("abs(1, 2)") == ParseError::Code::ArgumentCount);
        CHECK_CASE("an argument of the wrong type", code_of("abs(name)") == ParseError::Code::ArgumentType);
        CHECK_CASE("a number joined to a string", code_of("count + name") == ParseError::Code::MixedTypes);
        CHECK_CASE("arithmetic on a string", code_of("name * 2") == ParseError::Code::StringOperand);
        CHECK_CASE("null as an operand", code_of("null + null") == ParseError::Code::NullOperand);

        const Parser::result_type unterminated = parser.parse("count + 'zone", symbols);
        CHECK_CASE("an error has the position where parsing stopped and a message", !unterminated.has_value()
            && unterminated.error().position == 8 && !unterminated.error().message.empty());
        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("one expression evaluated as two types")
    {
        Parser parser;
        SymbolTable st_shared;
        st_shared["x"] = 1.5;
        st_shared["y"] = 2.5;

        Expression shared_expr = *parser.parse("x + y", st_shared);

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
