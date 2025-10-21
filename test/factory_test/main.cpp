
#include "quetzal/FactoryRegistering.hpp"
#include <string>
#include <memory>
#include <iostream>

#define NARGS 1

using namespace std;

class Base
{
public:

    virtual ~Base()
    {
    }

    virtual void show() = 0;
};

#if NARGS == 0
class D1 : public Base, public quetzal::FactoryRegistering<string, Base, D1>
#elif NARGS == 1
class D1 : public Base, public quetzal::FactoryRegistering<string, Base, D1, int>
#endif
{
public:

    static string factory_id()
    {
        return "D1";
    }

    D1() :
        m_i(0)
    {
    }

    explicit D1(int i) :
        m_i(i)
    {
    }

    virtual ~D1()
    {
    }

    virtual void show()
    {
        cout << "D1 " << m_i << endl;
    }

    int m_i;
};

#if NARGS == 0
class D2 : public Base, public quetzal::FactoryRegistering<string, Base, D2>
#elif NARGS == 1
class D2 : public Base, public quetzal::FactoryRegistering<string, Base, D2, int>
#endif
{
public:

    static string factory_id()
    {
        return "D2";
    }

    D2() :
        m_i(0)
    {
    }

    explicit D2(int i) :
        m_i(i)
    {
    }

    virtual ~D2()
    {
    }

    virtual void show()
    {
        cout << "D2 " << m_i << endl;
    }

    int m_i;
};

#if NARGS == 0
typedef quetzal::Factory<string, Base> BaseFactory_;
#elif NARGS == 1
typedef quetzal::Factory<string, Base, int> BaseFactory_;
#endif
typedef quetzal::SingletonHolder<BaseFactory_> BaseFactory;

int main(int argc, char* argv[])
{
    argc;
    argv;

    BaseFactory_& factory = BaseFactory::instance();

    cout << "registered ids:" << endl;
    cout << factory;

    try
    {
#if NARGS == 0
        shared_ptr<Base> pb1(factory.create(D1::factory_id()));
        shared_ptr<Base> pb2(factory.create(D2::factory_id()));
#elif NARGS == 1
        shared_ptr<Base> pb1(factory.create(D1::factory_id(), 1));
        shared_ptr<Base> pb2(factory.create(D2::factory_id(), 2));
#endif

        cout << "out:" << endl;
        pb1->show();
        pb2->show();
    }
    catch( const exception& e )
    {
        cerr << e.what() << endl;
    }

    char c;
    cin >> c;

    return 0;
}
