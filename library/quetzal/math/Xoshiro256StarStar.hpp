#if !defined(QUETZAL_MATH_XOSHIRO256STARSTAR_HPP)
#define QUETZAL_MATH_XOSHIRO256STARSTAR_HPP
//------------------------------------------------------------------------------
// math
// Xoshiro256StarStar.hpp
//------------------------------------------------------------------------------

#include <array>
#include <bit>
#include <cassert>
#include <cstdint>
#include <istream>
#include <limits>
#include <ostream>

namespace quetzal::math
{

    //--------------------------------------------------------------------------
    // xoshiro256** by David Blackman and Sebastiano Vigna, seeded through splitmix64 as its authors recommend
    class Xoshiro256StarStar
    {
    public:

        using result_type = std::uint64_t;
        using state_type = std::array<std::uint64_t, 4>;

        static constexpr result_type default_seed = 0;

        Xoshiro256StarStar();
        explicit Xoshiro256StarStar(result_type seed);
        explicit Xoshiro256StarStar(const state_type& state);
        Xoshiro256StarStar(const Xoshiro256StarStar&) = default;
        Xoshiro256StarStar(Xoshiro256StarStar&&) = default;
        ~Xoshiro256StarStar() = default;

        Xoshiro256StarStar& operator=(const Xoshiro256StarStar&) = default;
        Xoshiro256StarStar& operator=(Xoshiro256StarStar&&) = default;

        void seed(result_type seed = default_seed);

        static constexpr result_type min();
        static constexpr result_type max();

        result_type operator()();

        friend bool operator==(const Xoshiro256StarStar& a, const Xoshiro256StarStar& b) = default;

        // The state as four decimal numbers separated by spaces
        friend std::ostream& operator<<(std::ostream& os, const Xoshiro256StarStar& engine);

        // Leaves the engine unchanged if the stream fails
        friend std::istream& operator>>(std::istream& is, Xoshiro256StarStar& engine);

    private:

        state_type m_state;
    };

    std::ostream& operator<<(std::ostream& os, const Xoshiro256StarStar& engine);
    std::istream& operator>>(std::istream& is, Xoshiro256StarStar& engine);

    namespace detail
    {

        // Advances the state and returns the next value of Sebastiano Vigna's splitmix64
        std::uint64_t splitmix64(std::uint64_t& state);

    } // namespace detail

} // namespace quetzal::math

//------------------------------------------------------------------------------
inline quetzal::math::Xoshiro256StarStar::Xoshiro256StarStar() :
    Xoshiro256StarStar(default_seed)
{
}

//------------------------------------------------------------------------------
inline quetzal::math::Xoshiro256StarStar::Xoshiro256StarStar(result_type seed) :
    m_state()
{
    this->seed(seed);
}

//------------------------------------------------------------------------------
inline quetzal::math::Xoshiro256StarStar::Xoshiro256StarStar(const state_type& state) :
    m_state(state)
{
    assert(state != state_type{});
}

//------------------------------------------------------------------------------
inline void quetzal::math::Xoshiro256StarStar::seed(result_type seed)
{
    for (auto& word : m_state)
    {
        word = detail::splitmix64(seed);
    }
    return;
}

//------------------------------------------------------------------------------
constexpr quetzal::math::Xoshiro256StarStar::result_type quetzal::math::Xoshiro256StarStar::min()
{
    return std::numeric_limits<result_type>::min();
}

//------------------------------------------------------------------------------
constexpr quetzal::math::Xoshiro256StarStar::result_type quetzal::math::Xoshiro256StarStar::max()
{
    return std::numeric_limits<result_type>::max();
}

//------------------------------------------------------------------------------
inline quetzal::math::Xoshiro256StarStar::result_type quetzal::math::Xoshiro256StarStar::operator()()
{
    const result_type result = std::rotl(m_state[1] * 5, 7) * 9;
    const result_type t = m_state[1] << 17;

    m_state[2] ^= m_state[0];
    m_state[3] ^= m_state[1];
    m_state[1] ^= m_state[2];
    m_state[0] ^= m_state[3];
    m_state[2] ^= t;
    m_state[3] = std::rotl(m_state[3], 45);

    return result;
}

//------------------------------------------------------------------------------
inline std::ostream& quetzal::math::operator<<(std::ostream& os, const Xoshiro256StarStar& engine)
{
    return os << engine.m_state[0] << ' ' << engine.m_state[1] << ' ' << engine.m_state[2] << ' ' << engine.m_state[3];
}

//------------------------------------------------------------------------------
inline std::istream& quetzal::math::operator>>(std::istream& is, Xoshiro256StarStar& engine)
{
    Xoshiro256StarStar::state_type state;
    is >> state[0] >> state[1] >> state[2] >> state[3];

    if (is)
    {
        engine.m_state = state;
    }

    return is;
}

//------------------------------------------------------------------------------
inline std::uint64_t quetzal::math::detail::splitmix64(std::uint64_t& state)
{
    std::uint64_t z = (state += 0x9e3779b97f4a7c15);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
    z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
    return z ^ (z >> 31);
}

#endif // QUETZAL_MATH_XOSHIRO256STARSTAR_HPP
