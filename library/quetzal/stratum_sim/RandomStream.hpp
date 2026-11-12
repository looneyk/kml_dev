#if !defined(QUETZAL_STRATUM_SIM_RANDOMSTREAM_HPP)
#define QUETZAL_STRATUM_SIM_RANDOMSTREAM_HPP
//------------------------------------------------------------------------------
// stratum_sim
// RandomStream.hpp
//------------------------------------------------------------------------------

#include "types.hpp"
#include <concepts>
#include <iterator>
#include <mutex>
#include <ranges>
#include <string>
#include <string_view>

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    // A process-wide random engine, with math::Random's members for seeding, drawing and saving. Each member locks the
    // random engine for its whole draw, so threads drawing at once never corrupt its state
    class RandomStream
    {
    public:

        RandomStream() = default;
        RandomStream(const RandomStream&) = delete;
        RandomStream(RandomStream&&) = delete;
        ~RandomStream() = default;

        RandomStream& operator=(const RandomStream&) = delete;
        RandomStream& operator=(RandomStream&&) = delete;

        void seed(seed_type value);

        // Returns a seed drawn uniformly from the full range of seed values
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

        mutable std::mutex m_mutex;
        random_type m_random;
    };

    // The random engine that every draw during play uses
    RandomStream& gameplay_random();

} // namespace quetzal::stratum_sim

//------------------------------------------------------------------------------
template<std::integral T>
T quetzal::stratum_sim::RandomStream::random(T n)
{
    std::lock_guard lock(m_mutex);
    return m_random.random(n);
}

//------------------------------------------------------------------------------
template<std::floating_point T>
T quetzal::stratum_sim::RandomStream::random(T n)
{
    std::lock_guard lock(m_mutex);
    return m_random.random(n);
}

//------------------------------------------------------------------------------
template<std::integral T>
T quetzal::stratum_sim::RandomStream::random(T first, T last)
{
    std::lock_guard lock(m_mutex);
    return m_random.random(first, last);
}

//------------------------------------------------------------------------------
template<std::floating_point T>
T quetzal::stratum_sim::RandomStream::random(T first, T last)
{
    std::lock_guard lock(m_mutex);
    return m_random.random(first, last);
}

//------------------------------------------------------------------------------
template<std::integral T>
T quetzal::stratum_sim::RandomStream::random_closed(T n)
{
    std::lock_guard lock(m_mutex);
    return m_random.random_closed(n);
}

//------------------------------------------------------------------------------
template<std::floating_point T>
T quetzal::stratum_sim::RandomStream::random_closed(T n)
{
    std::lock_guard lock(m_mutex);
    return m_random.random_closed(n);
}

//------------------------------------------------------------------------------
template<std::integral T>
T quetzal::stratum_sim::RandomStream::random_closed(T first, T last)
{
    std::lock_guard lock(m_mutex);
    return m_random.random_closed(first, last);
}

//------------------------------------------------------------------------------
template<std::floating_point T>
T quetzal::stratum_sim::RandomStream::random_closed(T first, T last)
{
    std::lock_guard lock(m_mutex);
    return m_random.random_closed(first, last);
}

//------------------------------------------------------------------------------
template<std::ranges::random_access_range R> requires std::ranges::sized_range<R> && std::ranges::borrowed_range<R>
std::ranges::range_reference_t<R> quetzal::stratum_sim::RandomStream::choose(R&& range)
{
    std::lock_guard lock(m_mutex);
    return m_random.choose(std::forward<R>(range));
}

//------------------------------------------------------------------------------
template<std::ranges::random_access_range R>
void quetzal::stratum_sim::RandomStream::shuffle(R&& range)
{
    std::lock_guard lock(m_mutex);
    m_random.shuffle(std::forward<R>(range));
    return;
}

//------------------------------------------------------------------------------
template<std::random_access_iterator I, std::sentinel_for<I> S>
void quetzal::stratum_sim::RandomStream::shuffle(I first, S last)
{
    std::lock_guard lock(m_mutex);
    m_random.shuffle(first, last);
    return;
}

#endif // QUETZAL_STRATUM_SIM_RANDOMSTREAM_HPP
