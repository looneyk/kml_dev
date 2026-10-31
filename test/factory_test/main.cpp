//------------------------------------------------------------------------------
// factory_test
// main.cpp
//------------------------------------------------------------------------------

#include "quetzal/common/Exception.hpp"
#include "quetzal/common/Factory.hpp"
#include "quetzal/common/FactoryRegistering.hpp"
#include "quetzal/common/Singleton.hpp"
#include "quetzal/testing/named_checks.hpp"
#include <doctest/doctest.h>
#include <memory>
#include <sstream>
#include <string>

using namespace std;
using namespace quetzal;

namespace
{

    //--------------------------------------------------------------------------
    class Product
    {
    public:

        virtual ~Product() = default;

        virtual int value() const = 0;
    };

    using product_factory_type = Factory<string, Product, int>;
    using constant_product_factory_type = Factory<string, Product>;

    //--------------------------------------------------------------------------
    class DoublingProduct : public Product, public FactoryRegistering<string, Product, DoublingProduct, int>
    {
    public:

        static string factory_id()
        {
            return "doubling";
        }

        explicit DoublingProduct(int value) :
            m_value(value * 2)
        {
        }

        int value() const override
        {
            return m_value;
        }

    private:

        int m_value;
    };

    //--------------------------------------------------------------------------
    class TriplingProduct : public Product, public FactoryRegistering<string, Product, TriplingProduct, int>
    {
    public:

        static string factory_id()
        {
            return "tripling";
        }

        explicit TriplingProduct(int value) :
            m_value(value * 3)
        {
        }

        int value() const override
        {
            return m_value;
        }

    private:

        int m_value;
    };

    //--------------------------------------------------------------------------
    class ConstantProduct : public Product, public FactoryRegistering<string, Product, ConstantProduct>
    {
    public:

        static string factory_id()
        {
            return "constant";
        }

        int value() const override
        {
            return 7;
        }
    };

    //--------------------------------------------------------------------------
    struct Counter
    {
        int m_count = 0;
    };

    //--------------------------------------------------------------------------
    TEST_CASE("self-registration")
    {
        const auto& factory = SingletonHolder<product_factory_type>::instance();
        CHECK_CASE_EQUAL("each concrete class with an argument registers one creator", factory.associations().size(), 2u);
        CHECK_CASE("the doubling product is registered", factory.associations().contains(DoublingProduct::factory_id()));
        CHECK_CASE("the tripling product is registered", factory.associations().contains(TriplingProduct::factory_id()));

        const auto& constant_factory = SingletonHolder<constant_product_factory_type>::instance();
        CHECK_CASE_EQUAL("a concrete class without arguments registers in its own factory", constant_factory.associations().size(), 1u);
        CHECK_CASE("the constant product is registered", constant_factory.associations().contains(ConstantProduct::factory_id()));
        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("create")
    {
        const auto& factory = SingletonHolder<product_factory_type>::instance();
        CHECK_CASE_EQUAL("the doubling creator constructs a doubling product", factory.create("doubling", 5)->value(), 10);
        CHECK_CASE_EQUAL("the tripling creator constructs a tripling product", factory.create("tripling", 5)->value(), 15);
        CHECK_CASE("the created product has the concrete type", dynamic_pointer_cast<TriplingProduct>(factory.create("tripling", 1)) != nullptr);

        const auto& constant_factory = SingletonHolder<constant_product_factory_type>::instance();
        CHECK_CASE_EQUAL("a creator without arguments constructs its product", constant_factory.create("constant")->value(), 7);

        CHECK_THROWS_AS(factory.create("missing", 0), Exception);
        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("register_creator")
    {
        product_factory_type factory;
        const auto negating = [](int value)
        {
            return product_factory_type::result_type(make_shared<DoublingProduct>(-value));
        };

        CHECK_CASE("a new id is registered", factory.register_creator("negating", negating));
        CHECK_CASE("a registered id is not registered again", !factory.register_creator("negating", DoublingProduct::create));
        CHECK_CASE_EQUAL("the first creator remains", factory.create("negating", 4)->value(), -8);

        CHECK_CASE("a registered id is unregistered", factory.unregister_creator("negating"));
        CHECK_CASE("an unregistered id is not unregistered again", !factory.unregister_creator("negating"));
        CHECK_THROWS_AS(factory.create("negating", 4), Exception);

        CHECK_CASE("an unregistered id is registered again", factory.register_creator("negating", DoublingProduct::create));
        CHECK_CASE_EQUAL("and the new creator is used", factory.create("negating", 4)->value(), 8);
        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("operator<<")
    {
        ostringstream os;
        os << SingletonHolder<product_factory_type>::instance();
        CHECK_CASE_EQUAL("each registered id is written on its own line, in id order", os.str(), "doubling\ntripling\n");
        return;
    }

    //--------------------------------------------------------------------------
    TEST_CASE("singleton lifetimes")
    {
        auto& counter = SingletonHolder<Counter>::instance();
        ++counter.m_count;
        CHECK_CASE("the default lifetime returns one instance", &SingletonHolder<Counter>::instance() == &counter);
        CHECK_CASE_EQUAL("and the instance keeps its state", SingletonHolder<Counter>::instance().m_count, counter.m_count);

        auto& immortal_counter = SingletonHolder<Counter, ImmortalLifetime>::instance();
        ++immortal_counter.m_count;
        CHECK_CASE("the immortal lifetime returns one instance", &SingletonHolder<Counter, ImmortalLifetime>::instance() == &immortal_counter);
        CHECK_CASE("the immortal instance is separate from the default one", &immortal_counter != &counter);
        return;
    }

} // namespace
