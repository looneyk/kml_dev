#if !defined(QUETZAL_MATH_RANDOM_HPP)
#define QUETZAL_MATH_RANDOM_HPP
//------------------------------------------------------------------------------
// math
// Random.hpp
//------------------------------------------------------------------------------

#include <algorithm>
#include <array>
#include <cassert>
#include <charconv>
#include <format>
#include <iterator>
#include <limits>
#include <random>
#include <ranges>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace quetzal::math
{

    //--------------------------------------------------------------------------
    template<typename E = std::default_random_engine>
    class Random
    {
    public:

        Random();
        explicit Random(E::result_type seed);
        Random(const Random&) = delete;
        ~Random() = default;

        Random& operator=(const Random&) = delete;

        // Uniform distributions
        double random(); // [0, 1)

        template<typename T> requires std::is_integral_v<T>
        T random(T n); // [0, n)

        template<typename T> requires std::is_floating_point_v<T>
        T random(T n); // [0, n)

        template<typename T> requires std::is_integral_v<T>
        T random(T first, T last); // [first, last]

        template<typename T> requires std::is_floating_point_v<T>
        T random(T first, T last); // [first, last)

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

        E& engine();

    private:

        std::random_device m_rd;
        E m_engine;
    };

    //--------------------------------------------------------------------------
    template<typename E = std::default_random_engine>
    E::result_type to_seed(const std::string& s);

} // namespace quetzal::math

//------------------------------------------------------------------------------
template<typename E>
quetzal::math::Random<E>::Random() :
    m_rd(),
    m_engine(m_rd())
{
}

//------------------------------------------------------------------------------
template<typename E>
quetzal::math::Random<E>::Random(E::result_type seed) :
    m_rd(),
    m_engine(seed)
{
}

//------------------------------------------------------------------------------
template<typename E>
double quetzal::math::Random<E>::random()
{
    std::uniform_real_distribution<double> distribution(0.0, 1.0);
    return distribution(m_engine);
}

//------------------------------------------------------------------------------
template<typename E>
template<typename T> requires std::is_integral_v<T>
T quetzal::math::Random<E>::random(T n)
{
    std::uniform_int_distribution<T> distribution(T(0), T(n - 1));
    return distribution(m_engine);
}

//------------------------------------------------------------------------------
template<typename E>
template<typename T> requires std::is_floating_point_v<T>
T quetzal::math::Random<E>::random(T n)
{
    std::uniform_real_distribution<T> distribution(T(0), n);
    return distribution(m_engine);
}

//------------------------------------------------------------------------------
template<typename E>
template<typename T> requires std::is_integral_v<T>
T quetzal::math::Random<E>::random(T first, T last)
{
    std::uniform_int_distribution<T> distribution(first, last);
    return distribution(m_engine);
}

//------------------------------------------------------------------------------
template<typename E>
template<typename T> requires std::is_floating_point_v<T>
T quetzal::math::Random<E>::random(T first, T last)
{
    std::uniform_real_distribution<T> distribution(first, last);
    return distribution(m_engine);
}

//------------------------------------------------------------------------------
template<typename E>
double quetzal::math::Random<E>::gaussian(double mean, double sd)
{
    std::normal_distribution<double> distribution(mean, sd);
    return distribution(m_engine);
}

//------------------------------------------------------------------------------
template<typename E>
double quetzal::math::Random<E>::triangular(double mode, double halfwidth)
{
    std::array<double, 3> bounds {mode - halfwidth, mode, mode + halfwidth};
    std::array<double, 3> weights {0.0, 1.0, 0.0};
    std::piecewise_linear_distribution<double> distribution (bounds.begin(), bounds.end(), weights.begin());
    return distribution(m_engine);
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
}

//------------------------------------------------------------------------------
template<typename E>
template<std::random_access_iterator I, std::sentinel_for<I> S>
void quetzal::math::Random<E>::shuffle(I first, S last)
{
    std::shuffle(first, std::ranges::next(first, last), m_engine);
}

//------------------------------------------------------------------------------
template<typename E>
E& quetzal::math::Random<E>::engine()
{
    return m_engine;
}

//------------------------------------------------------------------------------
template<typename E>
E::result_type quetzal::math::to_seed(const std::string& s)
{
    unsigned long long value = 0;
    auto result = std::from_chars(s.data(), s.data() + s.size(), value);

    if (result.ec == std::errc::result_out_of_range)
    {
        throw std::invalid_argument(std::format("\"{}\" is too large for a seed", s));
    }

    if (result.ec != std::errc {} || result.ptr != s.data() + s.size())
    {
        throw std::invalid_argument(std::format("\"{}\" is not a whole number", s));
    }

    if (value > std::numeric_limits<typename E::result_type>::max())
    {
        throw std::invalid_argument(std::format("\"{}\" is larger than a seed can be", s));
    }

    return static_cast<typename E::result_type>(value);
}

#endif // QUETZAL_MATH_RANDOM_HPP
