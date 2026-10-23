//------------------------------------------------------------------------------
// stratum_sim
// RandomStream.cpp
//------------------------------------------------------------------------------

#include "RandomStream.hpp"

//------------------------------------------------------------------------------
void quetzal::stratum_sim::RandomStream::seed(seed_type value)
{
    std::lock_guard lock(m_mutex);
    m_random.seed(value);
    return;
}

//------------------------------------------------------------------------------
double quetzal::stratum_sim::RandomStream::random()
{
    std::lock_guard lock(m_mutex);
    return m_random.random();
}

//------------------------------------------------------------------------------
double quetzal::stratum_sim::RandomStream::gaussian(double mean, double sd)
{
    std::lock_guard lock(m_mutex);
    return m_random.gaussian(mean, sd);
}

//------------------------------------------------------------------------------
double quetzal::stratum_sim::RandomStream::triangular(double mode, double halfwidth)
{
    std::lock_guard lock(m_mutex);
    return m_random.triangular(mode, halfwidth);
}

//------------------------------------------------------------------------------
std::string quetzal::stratum_sim::RandomStream::state() const
{
    std::lock_guard lock(m_mutex);
    return m_random.state();
}

//------------------------------------------------------------------------------
void quetzal::stratum_sim::RandomStream::set_state(std::string_view text)
{
    std::lock_guard lock(m_mutex);
    m_random.set_state(text);
    return;
}

//------------------------------------------------------------------------------
quetzal::stratum_sim::RandomStream& quetzal::stratum_sim::gameplay_random()
{
    static RandomStream random_stream;
    return random_stream;
}
