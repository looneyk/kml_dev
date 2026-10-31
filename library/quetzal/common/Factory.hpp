#if !defined(QUETZAL_FACTORY_HPP)
#define QUETZAL_FACTORY_HPP
//------------------------------------------------------------------------------
// Factory.hpp
//------------------------------------------------------------------------------

#include <functional>
#include <map>
#include <memory>
#include <ostream>
#include <sstream>
#include <utility>
#include "Exception.hpp"

namespace quetzal
{

    //--------------------------------------------------------------------------
    template<typename IdType, typename BaseType, typename... Args>
    class Factory
    {
    public:

        typedef IdType id_type;
        typedef BaseType base_type;
        typedef std::shared_ptr<BaseType> result_type;
        typedef std::function<result_type(Args...)> creator_type;
        typedef std::map<id_type, creator_type> associations_type; // <id, creator>

        Factory() = default;
        Factory(const Factory& other) = delete;
        ~Factory() = default;

        Factory& operator=(const Factory& other) = delete;

        bool register_creator(const id_type& id, creator_type creator);
        bool unregister_creator(const id_type& id);

        result_type create(const id_type& id, Args... args) const;

        const associations_type& associations() const;

    private:

        associations_type m_associations;
    };

    template<typename IdType, typename BaseType, typename... Args>
    std::ostream& operator<<(std::ostream& os, const Factory<IdType, BaseType, Args...>& factory);

    //----------------------------------------------------------------------
    template<typename IdType, typename BaseType, typename... Args>
    bool Factory<IdType, BaseType, Args...>::register_creator(const id_type& id, creator_type creator)
    {
        return m_associations.emplace(id, std::move(creator)).second;
    }

    //----------------------------------------------------------------------
    template<typename IdType, typename BaseType, typename... Args>
    bool Factory<IdType, BaseType, Args...>::unregister_creator(const id_type& id)
    {
        return m_associations.erase(id) != 0;
    }

    //----------------------------------------------------------------------
    template<typename IdType, typename BaseType, typename... Args>
    typename Factory<IdType, BaseType, Args...>::result_type Factory<IdType, BaseType, Args...>::create(const id_type& id, Args... args) const
    {
        const auto association = m_associations.find(id);
        if (association == m_associations.end())
        {
            std::ostringstream oss;
            oss << "Factory::create - unknown id: " << id;
            throw Exception(oss.str(), __FILE__, __LINE__);
        }

        return association->second(std::forward<Args>(args)...);
    }

    //----------------------------------------------------------------------
    template<typename IdType, typename BaseType, typename... Args>
    const typename Factory<IdType, BaseType, Args...>::associations_type& Factory<IdType, BaseType, Args...>::associations() const
    {
        return m_associations;
    }

    //----------------------------------------------------------------------
    template<typename IdType, typename BaseType, typename... Args>
    std::ostream& operator<<(std::ostream& os, const Factory<IdType, BaseType, Args...>& factory)
    {
        for (const auto& association : factory.associations())
        {
            os << association.first << '\n';
        }

        return os;
    }

} // namespace quetzal

#endif // QUETZAL_FACTORY_HPP
