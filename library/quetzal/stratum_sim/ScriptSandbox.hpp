#if !defined(QUETZAL_STRATUM_SIM_SCRIPT_SANDBOX_HPP)
#define QUETZAL_STRATUM_SIM_SCRIPT_SANDBOX_HPP
//------------------------------------------------------------------------------
// stratum_sim
// ScriptSandbox.hpp
//------------------------------------------------------------------------------

#include "ScriptLimits.hpp"
#include "types.hpp"
#include "quetzal/common/KeyValueStore.hpp"
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

struct lua_State;
struct lua_Debug;

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // A Lua state that evaluates code with its globals and the functions and libraries that are safe to expose, within its limits
    class ScriptSandbox
    {
    public:

        // A global whose name has '.' is an entry of a table, as region.difficulty is of region. A global whose text is an
        // integer is an integer, one whose text is another number is a float, and any other is a string. The code's draws
        // come from the random engine, and the order in which pairs visits string keys from the world seed
        // Throws std::runtime_error if a global is an entry of another global that is not a table
        ScriptSandbox(const KeyValueStore& globals, random_type random, seed_type world_seed, const ScriptLimits& script_limits);
        ScriptSandbox(const ScriptSandbox&) = delete;
        ScriptSandbox(ScriptSandbox&&) = delete;
        ~ScriptSandbox();

        ScriptSandbox& operator=(const ScriptSandbox&) = delete;
        ScriptSandbox& operator=(ScriptSandbox&&) = delete;

        // Returns the result of the code execution
        // name identifies the code in error messages
        // Throws std::runtime_error if the code does not compile, fails, exceeds a limit, or has a table or a function as its value
        std::optional<std::string> evaluate(std::string_view name, std::string_view code);

    private:

        static void* allocate(void* user_data, void* block, std::size_t size_old, std::size_t size);
        static void count_instructions(lua_State* state, lua_Debug* debug);
        static int random(lua_State* state);
        static ScriptSandbox& sandbox(lua_State* state);

        void create_environment(const KeyValueStore& globals);
        void push_library();

        random_type m_random;
        ScriptLimits m_script_limits;
        std::size_t m_memory_size;
        std::size_t m_instruction_count;
        lua_State* m_state;
        int m_environment; // A reference in the registry
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_SCRIPT_SANDBOX_HPP
