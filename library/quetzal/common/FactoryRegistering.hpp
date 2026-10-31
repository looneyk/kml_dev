#if !defined(QUETZAL_FACTORYREGISTERING_HPP)
#define QUETZAL_FACTORYREGISTERING_HPP
//------------------------------------------------------------------------------
// FactoryRegistering.hpp
// Policy class for factory self-registration
//------------------------------------------------------------------------------

#include <concepts>
#include <memory>
#include <utility>
#include "Factory.hpp"
#include "Singleton.hpp"

namespace quetzal
{

    //--------------------------------------------------------------------------
    template<typename IdType, typename BaseType, typename ConcreteType, typename... Args>
    class FactoryRegistering
    {
    public:

        typedef IdType id_type;
        typedef ConcreteType concrete_type;
        typedef BaseType base_type;

        static std::shared_ptr<base_type> create(Args... args);

    protected:

        FactoryRegistering() = default;
        FactoryRegistering(const FactoryRegistering& other) = delete;
        virtual ~FactoryRegistering();

        FactoryRegistering& operator=(const FactoryRegistering& other) = delete;

    private:

        typedef SingletonHolder<Factory<IdType, BaseType, Args...>> factory_singleton_type;

        static const bool s_registered;
    };

    //--------------------------------------------------------------------------
    template<typename IdType, typename BaseType, typename ConcreteType, typename... Args>
    std::shared_ptr<BaseType> FactoryRegistering<IdType, BaseType, ConcreteType, Args...>::create(Args... args)
    {
        static_assert(std::derived_from<concrete_type, base_type>);

        return std::make_shared<concrete_type>(std::forward<Args>(args)...);
    }

    //--------------------------------------------------------------------------
    template<typename IdType, typename BaseType, typename ConcreteType, typename... Args>
    FactoryRegistering<IdType, BaseType, ConcreteType, Args...>::~FactoryRegistering()
    {
        // Odr-using s_registered instantiates it, and its initialization registers the creator.
        static_cast<void>(s_registered);
    }

    //--------------------------------------------------------------------------
    template<typename IdType, typename BaseType, typename ConcreteType, typename... Args>
    const bool FactoryRegistering<IdType, BaseType, ConcreteType, Args...>::s_registered =
        factory_singleton_type::instance().register_creator(ConcreteType::factory_id(), create);

} // namespace quetzal

#endif // QUETZAL_FACTORYREGISTERING_HPP
