#if !defined(QUETZAL_SINGLETON_HPP)
#define QUETZAL_SINGLETON_HPP
//------------------------------------------------------------------------------
// Singleton.hpp
//------------------------------------------------------------------------------

namespace quetzal
{

    //--------------------------------------------------------------------------
    template<typename T>
    struct DefaultLifetime
    {
        static T& instance()
        {
            static T s_instance;
            return s_instance;
        }
    };

    //--------------------------------------------------------------------------
    // The instance is never destroyed, so it remains valid throughout static destruction.
    template<typename T>
    struct ImmortalLifetime
    {
        static T& instance()
        {
            static Storage s_storage;
            return s_storage.m_instance;
        }

    private:

        union Storage
        {
            Storage() :
                m_instance()
            {
            }

            ~Storage()
            {
            }

            T m_instance;
        };
    };

    //--------------------------------------------------------------------------
    template<typename T, template<typename> typename LifetimePolicy = DefaultLifetime>
    class SingletonHolder
    {
    public:

        SingletonHolder() = delete;

        static T& instance()
        {
            return LifetimePolicy<T>::instance();
        }
    };

} // namespace quetzal

#endif // QUETZAL_SINGLETON_HPP
