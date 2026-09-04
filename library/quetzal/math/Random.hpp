#if !defined(QUETZAL_MATH_RANDOM_HPP)
#define QUETZAL_MATH_RANDOM_HPP
//------------------------------------------------------------------------------
// math
// Random.hpp
//------------------------------------------------------------------------------

#include <format>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>

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

        // uniform distribution
        double gen(); // [0, 1)

        template<typename T>
        T gen(T n); // [0, n)

        template<typename T>
        T gen(T first, T last); // [first, last] for integral, [first, last) for floating point

        // normal distribution
        double genn(double mean, double stddev);

        // trianglar distribution
        double gent(double mode, double halfwidth);

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
double quetzal::math::Random<E>::gen()
{
    std::uniform_real_distribution<double> distribution(0.0, 1.0);
    return distribution(m_engine);
}

//------------------------------------------------------------------------------
template<typename E>
template<typename T>
T quetzal::math::Random<E>::gen(T n)
{
    if constexpr (std::is_integral_v<T>)
    {
        std::uniform_int_distribution<T> distribution(T(0), T(n - 1));
        return distribution(m_engine);
    }
    else if constexpr (std::is_floating_point_v<T>)
    {
        std::uniform_real_distribution<T> distribution(T(0), n);
        return distribution(m_engine);
    }
}

//------------------------------------------------------------------------------
template<typename E>
template<typename T>
T quetzal::math::Random<E>::gen(T first, T last)
{
    if constexpr (std::is_integral_v<T>)
    {
        std::uniform_int_distribution<T> distribution(first, last);
        return distribution(m_engine);
    }
    else if constexpr (std::is_floating_point_v<T>)
    {
        std::uniform_real_distribution<T> distribution(first, last);
        return distribution(m_engine);
    }
}

//------------------------------------------------------------------------------
template<typename E>
double quetzal::math::Random<E>::genn(double mean, double stddev)
{
    std::normal_distribution<double> distribution(mean, stddev);
    return distribution(m_engine);
}

//------------------------------------------------------------------------------
template<typename E>
double quetzal::math::Random<E>::gent(double mode, double halfwidth)
{
    std::array<double, 3> bounds {mode - halfwidth, mode, mode + halfwidth};
    std::array<double, 3> weights {0.0, 1.0, 0.0};
    std::piecewise_linear_distribution<double> distribution (bounds.begin(), bounds.end(), weights.begin());
    return distribution(m_engine);
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
    size_t n = 0;
    unsigned long long value = std::stoull(s, &n);

    if (n != s.size())
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
