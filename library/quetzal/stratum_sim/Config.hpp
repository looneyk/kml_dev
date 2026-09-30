#if !defined(QUETZAL_STRATUM_SIM_CONFIG_HPP)
#define QUETZAL_STRATUM_SIM_CONFIG_HPP
//------------------------------------------------------------------------------
// stratum_sim
// Config.hpp
//------------------------------------------------------------------------------

namespace quetzal::stratum_sim
{

    //--------------------------------------------------------------------------
    class Config
    {
    public:

        Config();
        Config(const Config&) = default;
        Config(Config&&) = default;
        ~Config() = default;

        Config& operator=(const Config&) = default;
        Config& operator=(Config&&) = default;

    private:

        // ...

        int m_placeholder;
    };

} // namespace quetzal::stratum_sim

#endif // QUETZAL_STRATUM_SIM_CONFIG_HPP
