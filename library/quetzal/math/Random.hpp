#if !defined(QUETZAL_MATH_RANDOM_HPP)
#define QUETZAL_MATH_RANDOM_HPP
//------------------------------------------------------------------------------
// math
// Random.hpp
//------------------------------------------------------------------------------

#include "Xoshiro256StarStar.hpp"
#include <algorithm>
#include <cassert>
#include <charconv>
#include <cmath>
#include <concepts>
#include <cstdint>
#include <format>
#include <iterator>
#include <limits>
#include <random>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

namespace quetzal::math
{

    using portable_random_engine_type = Xoshiro256StarStar;

    //--------------------------------------------------------------------------
    template<typename E = portable_random_engine_type>
    class Random
    {
        static_assert(E::min() == 0 && E::max() == std::numeric_limits<std::uint64_t>::max(), "Random draws 64 uniform bits from its random engine");

    public:

        // The seed of every random engine, independent of the type of its results
        using seed_type = std::uint64_t;

        Random();
        explicit Random(seed_type seed);
        Random(const Random&) = delete;
        Random(Random&&) = default;
        ~Random() = default;

        Random& operator=(const Random&) = delete;
        Random& operator=(Random&&) = default;

        void seed(seed_type value);

        // Returns a seed drawn uniformly from every value of seed_type
        seed_type seed();

        // Uniform distributions
        double random(); // [0, 1)

        template<std::integral T>
        T random(T n); // [0, n)

        template<std::floating_point T>
        T random(T n); // [0, n)

        template<std::integral T>
        T random(T first, T last); // [first, last)

        template<std::floating_point T>
        T random(T first, T last); // [first, last)

        template<std::integral T>
        T random_closed(T n); // [0, n]

        template<std::floating_point T>
        T random_closed(T n); // [0, n]

        template<std::integral T>
        T random_closed(T first, T last); // [first, last]

        template<std::floating_point T>
        T random_closed(T first, T last); // [first, last]

        // Normal distribution
        double gaussian(double mean, double sd);

        // Triangular distribution
        double triangular(double mode, double halfwidth);

        template<std::ranges::random_access_range R> requires std::ranges::sized_range<R> && std::ranges::borrowed_range<R>
        std::ranges::range_reference_t<R> choose(R&& range);

        template<std::ranges::random_access_range R>
        void shuffle(R&& range);

        template<std::random_access_iterator I, std::sentinel_for<I> S>
        void shuffle(I first, S last);

        // The random engine's state as text
        std::string state() const;

        // Throws std::invalid_argument if the text is not a state of the random engine
        void set_state(std::string_view text);

    private:

        std::uint64_t bounded(std::uint64_t range); // [0, range)

        template<std::floating_point T>
        T unit(); // [0, 1)

        template<std::floating_point T>
        T unit_closed(); // [0, 1]

        E m_engine;
    };

    //--------------------------------------------------------------------------
    template<typename E = portable_random_engine_type>
    Random<E>::seed_type to_seed(const std::string& s);

    //--------------------------------------------------------------------------
    // A seed drawn from the platform's entropy source
    template<typename E = portable_random_engine_type>
    Random<E>::seed_type random_seed();

    namespace detail
    {

        // Returns the high 64 bits of the 128-bit product, and sets low to its low 64 bits
        std::uint64_t multiply_wide(std::uint64_t a, std::uint64_t b, std::uint64_t& low);

    } // namespace detail

} // namespace quetzal::math

//------------------------------------------------------------------------------
template<typename E>
quetzal::math::Random<E>::Random() :
    m_engine(random_seed<E>())
{
}

//------------------------------------------------------------------------------
template<typename E>
quetzal::math::Random<E>::Random(seed_type seed) :
    m_engine(seed)
{
}

//------------------------------------------------------------------------------
template<typename E>
void quetzal::math::Random<E>::seed(seed_type value)
{
    m_engine.seed(value);
    return;
}

//------------------------------------------------------------------------------
template<typename E>
quetzal::math::Random<E>::seed_type quetzal::math::Random<E>::seed()
{
    return random_closed(std::numeric_limits<seed_type>::max());
}

//------------------------------------------------------------------------------
template<typename E>
double quetzal::math::Random<E>::random()
{
    return unit<double>();
}

//------------------------------------------------------------------------------
template<typename E>
template<std::integral T>
T quetzal::math::Random<E>::random(T n)
{
    assert(n > T(0));
    return static_cast<T>(bounded(static_cast<std::uint64_t>(n)));
}

//------------------------------------------------------------------------------
template<typename E>
template<std::floating_point T>
T quetzal::math::Random<E>::random(T n)
{
    assert(n > T(0));

    T value = n;
    while (value >= n)
    {
        value = n * unit<T>();
    }

    return value;
}

//------------------------------------------------------------------------------
template<typename E>
template<std::integral T>
T quetzal::math::Random<E>::random(T first, T last)
{
    assert(first < last);

    using unsigned_type = std::make_unsigned_t<T>;
    const std::uint64_t span = static_cast<unsigned_type>(static_cast<unsigned_type>(last) - static_cast<unsigned_type>(first));
    return static_cast<T>(static_cast<unsigned_type>(static_cast<unsigned_type>(first) + static_cast<unsigned_type>(bounded(span))));
}

//------------------------------------------------------------------------------
template<typename E>
template<std::floating_point T>
T quetzal::math::Random<E>::random(T first, T last)
{
    assert(first < last);
    assert(std::isfinite(last - first));

    T value = last;
    while (value >= last)
    {
        value = first + (last - first) * unit<T>();
    }

    return value;
}

//------------------------------------------------------------------------------
template<typename E>
template<std::integral T>
T quetzal::math::Random<E>::random_closed(T n)
{
    assert(n >= T(0));
    return random_closed(T(0), n);
}

//------------------------------------------------------------------------------
template<typename E>
template<std::floating_point T>
T quetzal::math::Random<E>::random_closed(T n)
{
    assert(n >= T(0));
    return n * unit_closed<T>();
}

//------------------------------------------------------------------------------
template<typename E>
template<std::integral T>
T quetzal::math::Random<E>::random_closed(T first, T last)
{
    assert(first <= last);

    using unsigned_type = std::make_unsigned_t<T>;
    const std::uint64_t span = static_cast<unsigned_type>(static_cast<unsigned_type>(last) - static_cast<unsigned_type>(first));
    const std::uint64_t offset = span == std::numeric_limits<std::uint64_t>::max() ? m_engine() : bounded(span + 1);
    return static_cast<T>(static_cast<unsigned_type>(static_cast<unsigned_type>(first) + static_cast<unsigned_type>(offset)));
}

//------------------------------------------------------------------------------
template<typename E>
template<std::floating_point T>
T quetzal::math::Random<E>::random_closed(T first, T last)
{
    assert(first <= last);
    assert(std::isfinite(last - first));

    return std::min(first + (last - first) * unit_closed<T>(), last);
}

//------------------------------------------------------------------------------
template<typename E>
double quetzal::math::Random<E>::gaussian(double mean, double sd)
{
    // Marsaglia's polar method
    double u = 0.0;
    double v = 0.0;
    double s = 0.0;

    do
    {
        u = 2.0 * random() - 1.0;
        v = 2.0 * random() - 1.0;
        s = u * u + v * v;
    }
    while (s >= 1.0 || s == 0.0);

    return mean + sd * u * std::sqrt(-2.0 * std::log(s) / s);
}

//------------------------------------------------------------------------------
template<typename E>
double quetzal::math::Random<E>::triangular(double mode, double halfwidth)
{
    const double u = random();

    if (u < 0.5)
    {
        return mode - halfwidth + halfwidth * std::sqrt(2.0 * u);
    }

    return mode + halfwidth - halfwidth * std::sqrt(2.0 * (1.0 - u));
}

//------------------------------------------------------------------------------
template<typename E>
template<std::ranges::random_access_range R> requires std::ranges::sized_range<R> && std::ranges::borrowed_range<R>
std::ranges::range_reference_t<R> quetzal::math::Random<E>::choose(R&& range)
{
    assert(!std::ranges::empty(range));
    return std::ranges::begin(range)[static_cast<std::ranges::range_difference_t<R>>(random(std::ranges::size(range)))];
}

//------------------------------------------------------------------------------
template<typename E>
template<std::ranges::random_access_range R>
void quetzal::math::Random<E>::shuffle(R&& range)
{
    shuffle(std::ranges::begin(range), std::ranges::end(range));
    return;
}

//------------------------------------------------------------------------------
template<typename E>
template<std::random_access_iterator I, std::sentinel_for<I> S>
void quetzal::math::Random<E>::shuffle(I first, S last)
{
    // Fisher-Yates
    const I end = std::ranges::next(first, last);

    for (auto i = end - first - 1; i > 0; --i)
    {
        std::ranges::iter_swap(first + i, first + random(i + 1));
    }

    return;
}

//------------------------------------------------------------------------------
template<typename E>
std::string quetzal::math::Random<E>::state() const
{
    std::ostringstream os;
    os << m_engine;
    return os.str();
}

//------------------------------------------------------------------------------
template<typename E>
void quetzal::math::Random<E>::set_state(std::string_view text)
{
    std::istringstream is{std::string(text)};
    E engine;
    char trailing = 0;

    if (!(is >> engine) || is >> trailing)
    {
        throw std::invalid_argument(std::format("\"{}\" is not a state of the random engine", text));
    }

    m_engine = engine;
    return;
}

//------------------------------------------------------------------------------
template<typename E>
std::uint64_t quetzal::math::Random<E>::bounded(std::uint64_t range)
{
    // Lemire's method, which rejects only the draws that would bias the result
    std::uint64_t low = 0;
    std::uint64_t high = detail::multiply_wide(m_engine(), range, low);

    if (low < range)
    {
        const std::uint64_t threshold = (std::uint64_t(0) - range) % range;

        while (low < threshold)
        {
            high = detail::multiply_wide(m_engine(), range, low);
        }
    }

    return high;
}

//------------------------------------------------------------------------------
template<typename E>
template<std::floating_point T>
T quetzal::math::Random<E>::unit()
{
    constexpr int digits = std::numeric_limits<T>::digits < 64 ? std::numeric_limits<T>::digits : 64;
    return static_cast<T>(m_engine() >> (64 - digits)) * std::ldexp(T(1), -digits);
}

//------------------------------------------------------------------------------
template<typename E>
template<std::floating_point T>
T quetzal::math::Random<E>::unit_closed()
{
    // 2^digits + 1 values, so the count fits in 64 bits only below 64 digits
    constexpr int digits = std::numeric_limits<T>::digits < 63 ? std::numeric_limits<T>::digits : 63;
    return static_cast<T>(bounded((std::uint64_t(1) << digits) + 1)) * std::ldexp(T(1), -digits);
}

//------------------------------------------------------------------------------
template<typename E>
quetzal::math::Random<E>::seed_type quetzal::math::to_seed(const std::string& s)
{
    using seed_type = Random<E>::seed_type;

    unsigned long long value = 0;
    auto result = std::from_chars(s.data(), s.data() + s.size(), value);

    if (result.ec == std::errc::result_out_of_range)
    {
        throw std::invalid_argument(std::format("\"{}\" is too large for a seed", s));
    }

    if (result.ec != std::errc{} || result.ptr != s.data() + s.size())
    {
        throw std::invalid_argument(std::format("\"{}\" is not a whole number", s));
    }

    if (value > std::numeric_limits<seed_type>::max())
    {
        throw std::invalid_argument(std::format("\"{}\" is larger than a seed can be", s));
    }

    return static_cast<seed_type>(value);
}

//------------------------------------------------------------------------------
template<typename E>
quetzal::math::Random<E>::seed_type quetzal::math::random_seed()
{
    using seed_type = Random<E>::seed_type;

    std::random_device device;

    if constexpr (std::numeric_limits<seed_type>::digits > std::numeric_limits<std::random_device::result_type>::digits)
    {
        return (static_cast<seed_type>(device()) << std::numeric_limits<std::random_device::result_type>::digits) | device();
    }
    else
    {
        return static_cast<seed_type>(device());
    }
}

//------------------------------------------------------------------------------
inline std::uint64_t quetzal::math::detail::multiply_wide(std::uint64_t a, std::uint64_t b, std::uint64_t& low)
{
    const std::uint64_t a_low = a & 0xffffffff;
    const std::uint64_t a_high = a >> 32;
    const std::uint64_t b_low = b & 0xffffffff;
    const std::uint64_t b_high = b >> 32;

    const std::uint64_t low_low = a_low * b_low;
    const std::uint64_t high_low = a_high * b_low;
    const std::uint64_t low_high = a_low * b_high;
    const std::uint64_t middle = (low_low >> 32) + (high_low & 0xffffffff) + low_high;

    low = (middle << 32) | (low_low & 0xffffffff);
    return a_high * b_high + (high_low >> 32) + (middle >> 32);
}

#endif // QUETZAL_MATH_RANDOM_HPP
