//------------------------------------------------------------------------------
// stratum_sim
// ScriptSandbox.cpp
//------------------------------------------------------------------------------

#include "ScriptSandbox.hpp"
#include "quetzal/common/string_util.hpp"
#include "quetzal/log/Log.hpp"
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <algorithm>
#include <cstdlib>
#include <format>
#include <initializer_list>
#include <optional>
#include <stdexcept>
#include <utility>

namespace
{

    constexpr int hook_interval = 1000;

    //--------------------------------------------------------------------------
    [[noreturn]] void fail(const std::string& message)
    {
        quetzal::log::logger().error("stratum_sim", "{}", message);
        throw std::runtime_error(message);
    }

    //--------------------------------------------------------------------------
    std::string error_text(lua_State* state)
    {
        const char* text = lua_tostring(state, -1);
        return text != nullptr ? text : "an error whose value is not a string";
    }

    //--------------------------------------------------------------------------
    // The seed of a state's string hashes, which decides the order in which pairs visits string keys
    unsigned hash_seed(std::uint64_t world_seed)
    {
        return static_cast<unsigned>(world_seed ^ (world_seed >> 32));
    }

    //--------------------------------------------------------------------------
    int reject_assignment(lua_State* state)
    {
        return luaL_error(state, "a library table is read-only");
    }

    //--------------------------------------------------------------------------
    int reject_unknown_name(lua_State* state)
    {
        return luaL_error(state, "the name %s is not defined", lua_tostring(state, 2));
    }

    //--------------------------------------------------------------------------
    // Pushes a read-only table with the entries of the global table library, excluding the keys excluded
    void push_read_only_library(lua_State* state, const char* library, std::initializer_list<std::string_view> excluded)
    {
        lua_getglobal(state, library);
        lua_newtable(state);
        lua_pushnil(state);
        while (lua_next(state, -3) != 0)
        {
            const bool kept = lua_type(state, -2) != LUA_TSTRING || std::ranges::find(excluded, std::string_view(lua_tostring(state, -2))) == excluded.end();
            if (kept)
            {
                lua_pushvalue(state, -2);
                lua_insert(state, -2);
                lua_rawset(state, -4);
            }
            else
            {
                lua_pop(state, 1);
            }
        }

        lua_newtable(state);
        lua_newtable(state);
        lua_pushvalue(state, -3);
        lua_setfield(state, -2, "__index");
        lua_pushcfunction(state, reject_assignment);
        lua_setfield(state, -2, "__newindex");
        lua_pushboolean(state, 0);
        lua_setfield(state, -2, "__metatable");
        lua_setmetatable(state, -2);
        lua_replace(state, -3);
        lua_pop(state, 1);
        return;
    }

    //--------------------------------------------------------------------------
    void push_global_value(lua_State* state, std::string_view text)
    {
        if (const std::optional<lua_Integer> integer = quetzal::try_type<lua_Integer>(text); integer.has_value())
        {
            lua_pushinteger(state, integer.value());
        }
        else if (const std::optional<double> number = quetzal::try_type<double>(text); number.has_value())
        {
            lua_pushnumber(state, number.value());
        }
        else
        {
            lua_pushlstring(state, text.data(), text.size());
        }

        return;
    }

    //--------------------------------------------------------------------------
    // Sets the global in the table at the index, a name with '.' setting an entry of a table in it
    void set_global(lua_State* state, int table, std::string_view name, std::string_view value)
    {
        lua_pushvalue(state, table);
        std::size_t start = 0;
        for (std::size_t dot = name.find('.'); dot != std::string_view::npos; dot = name.find('.', start))
        {
            const std::string_view part = name.substr(start, dot - start);
            lua_pushlstring(state, part.data(), part.size());
            const int type = lua_rawget(state, -2);
            if (type == LUA_TNIL)
            {
                lua_pop(state, 1);
                lua_newtable(state);
                lua_pushlstring(state, part.data(), part.size());
                lua_pushvalue(state, -2);
                lua_rawset(state, -4);
            }
            else if (type != LUA_TTABLE)
            {
                lua_pop(state, 2);
                fail(std::format("the global {} is an entry of {}, which is not a table", name, name.substr(0, dot)));
            }

            lua_remove(state, -2);
            start = dot + 1;
        }

        const std::string_view last = name.substr(start);
        lua_pushlstring(state, last.data(), last.size());
        push_global_value(state, value);
        lua_rawset(state, -3);
        lua_pop(state, 1);
        return;
    }

} // namespace

//------------------------------------------------------------------------------
quetzal::stratum_sim::ScriptSandbox::ScriptSandbox(const KeyValueStore& globals, random_type random, std::uint64_t world_seed, const ScriptLimits& script_limits) :
    m_random(std::move(random)),
    m_script_limits(script_limits),
    m_memory_size(0),
    m_instruction_count(0),
    m_state(lua_newstate(allocate, this, hash_seed(world_seed))),
    m_environment(LUA_NOREF)
{
    if (m_state == nullptr)
    {
        fail("a Lua state cannot be created");
    }

    *static_cast<ScriptSandbox**>(lua_getextraspace(m_state)) = this;
    luaL_openselectedlibs(m_state, LUA_GLIBK | LUA_MATHLIBK | LUA_STRLIBK | LUA_TABLIBK | LUA_UTF8LIBK, 0);
    lua_gc(m_state, LUA_GCSTOP);
    lua_sethook(m_state, count_instructions, LUA_MASKCOUNT, hook_interval);
    create_environment(globals);
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::ScriptSandbox::~ScriptSandbox()
{
    lua_close(m_state);
}

//------------------------------------------------------------------------------
std::optional<std::string> quetzal::stratum_sim::ScriptSandbox::evaluate(std::string_view name, std::string_view code)
{
    const std::string chunk_name = std::format("={}", name);
    const std::string expression = std::format("return {}", code);
    if (luaL_loadbufferx(m_state, expression.data(), expression.size(), chunk_name.c_str(), "t") != LUA_OK)
    {
        lua_pop(m_state, 1);
        if (luaL_loadbufferx(m_state, code.data(), code.size(), chunk_name.c_str(), "t") != LUA_OK)
        {
            const std::string message = std::format("the code '{}' of {} does not compile: {}", code, name, error_text(m_state));
            lua_pop(m_state, 1);
            fail(message);
        }
    }

    lua_rawgeti(m_state, LUA_REGISTRYINDEX, m_environment);
    lua_setupvalue(m_state, -2, 1);

    m_instruction_count = 0;
    if (lua_pcall(m_state, 0, 1, 0) != LUA_OK)
    {
        const std::string message = std::format("the code '{}' of {} fails: {}", code, name, error_text(m_state));
        lua_pop(m_state, 1);
        fail(message);
    }

    std::optional<std::string> value;
    switch (lua_type(m_state, -1))
    {
        case LUA_TNIL:
            break;

        case LUA_TBOOLEAN:
            value = lua_toboolean(m_state, -1) != 0 ? "true" : "false";
            break;

        case LUA_TNUMBER:
            value = lua_isinteger(m_state, -1) ? std::format("{}", lua_tointeger(m_state, -1)) : std::format("{}", lua_tonumber(m_state, -1));
            break;

        case LUA_TSTRING:
        {
            std::size_t size = 0;
            const char* text = lua_tolstring(m_state, -1, &size);
            value = std::string(text, size);
            break;
        }

        default:
        {
            const std::string message = std::format("the code '{}' of {} has a value that is a {}, not a boolean, a number, a string, or nil",
                code, name, luaL_typename(m_state, -1));
            lua_pop(m_state, 1);
            fail(message);
        }
    }

    lua_pop(m_state, 1);
    return value;
}

//------------------------------------------------------------------------------
void* quetzal::stratum_sim::ScriptSandbox::allocate(void* user_data, void* block, std::size_t size_old, std::size_t size)
{
    ScriptSandbox& allocating = *static_cast<ScriptSandbox*>(user_data);
    const std::size_t size_allocated = block != nullptr ? size_old : 0;
    if (size == 0)
    {
        allocating.m_memory_size -= size_allocated;
        std::free(block);
        return nullptr;
    }

    if (size > size_allocated && allocating.m_memory_size + (size - size_allocated) > allocating.m_script_limits.memory_size_max)
    {
        return nullptr;
    }

    void* reallocated = std::realloc(block, size);
    if (reallocated != nullptr)
    {
        allocating.m_memory_size = allocating.m_memory_size - size_allocated + size;
    }

    return reallocated;
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::ScriptSandbox::count_instructions(lua_State* state, lua_Debug*)
{
    ScriptSandbox& counting = sandbox(state);
    counting.m_instruction_count += hook_interval;
    if (counting.m_instruction_count > counting.m_script_limits.instruction_count_max)
    {
        luaL_error(state, "the limit of %I instructions is exceeded", static_cast<lua_Integer>(counting.m_script_limits.instruction_count_max));
    }

    return;
}

//------------------------------------------------------------------------------
// The interface of Lua's math.random
int quetzal::stratum_sim::ScriptSandbox::random(lua_State* state)
{
    random_type& drawn = sandbox(state).m_random;
    lua_Integer first = 1;
    lua_Integer last = 0;
    switch (lua_gettop(state))
    {
        case 0:
            lua_pushnumber(state, drawn.random());
            return 1;

        case 1:
            last = luaL_checkinteger(state, 1);
            if (last == 0)
            {
                lua_pushinteger(state, drawn.random_closed(LUA_MININTEGER, LUA_MAXINTEGER));
                return 1;
            }
            break;

        case 2:
            first = luaL_checkinteger(state, 1);
            last = luaL_checkinteger(state, 2);
            break;

        default:
            return luaL_error(state, "wrong number of arguments");
    }

    luaL_argcheck(state, first <= last, 1, "interval is empty");
    lua_pushinteger(state, drawn.random_closed(first, last));
    return 1;
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::ScriptSandbox& quetzal::stratum_sim::ScriptSandbox::sandbox(lua_State* state)
{
    return **static_cast<ScriptSandbox**>(lua_getextraspace(state));
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::ScriptSandbox::create_environment(const KeyValueStore& globals)
{
    lua_newtable(m_state);
    const int environment = lua_gettop(m_state);
    for (const auto& [name, value] : globals)
    {
        set_global(m_state, environment, name, value);
    }

    lua_newtable(m_state);
    push_library();
    lua_setfield(m_state, -2, "__index");
    lua_setmetatable(m_state, environment);
    m_environment = luaL_ref(m_state, LUA_REGISTRYINDEX);
    return;
}

//------------------------------------------------------------------------------
// Pushes the table of the functions and libraries that code can use, in which a name that is not defined is an error
void quetzal::stratum_sim::ScriptSandbox::push_library()
{
    lua_newtable(m_state);
    for (const char* name : {"assert", "error", "ipairs", "next", "pairs", "pcall", "rawequal", "rawlen", "select", "setmetatable", "tonumber",
        "tostring", "type", "xpcall"})
    {
        lua_getglobal(m_state, name);
        lua_setfield(m_state, -2, name);
    }

    push_read_only_library(m_state, "math", {"random", "randomseed"});
    lua_setfield(m_state, -2, "math");
    push_read_only_library(m_state, "string", {"dump"});
    lua_setfield(m_state, -2, "string");
    push_read_only_library(m_state, "table", {});
    lua_setfield(m_state, -2, "table");
    push_read_only_library(m_state, "utf8", {});
    lua_setfield(m_state, -2, "utf8");
    lua_pushcfunction(m_state, random);
    lua_setfield(m_state, -2, "random");

    lua_newtable(m_state);
    lua_pushcfunction(m_state, reject_unknown_name);
    lua_setfield(m_state, -2, "__index");
    lua_setmetatable(m_state, -2);
    return;
}
