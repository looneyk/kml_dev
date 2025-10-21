#if !defined(QUETZAL_MATH_PARAMETRIC_HPP)
#define QUETZAL_MATH_PARAMETRIC_HPP
//------------------------------------------------------------------------------
// math
// parametric.hpp
// Facility for constructing higher-order parametric functions
//------------------------------------------------------------------------------

#include <functional>

namespace quetzal::math
{

    // restrict, extend, and select all perform the same operation, but are included for clarity
    // should parametric functions allow for a distinction between domain and range data types? ...
    // These names, especially scale, are too generic, probably need some context in names ...

    // Given a function f with a domain of [0, 1], returns a function g with a domain of [0..1] which maps to the domain [t0, t1] of f

    // Map the function f domain t0..t1 to the returned function domain 0..1
    // Conceptually 0 <= t0 <= t1 <= 1 but this is not enforced
    template<typename T>
    std::function<T(const T&)> restrict(const T& t0, const T& t1, std::function<T(const T&)> f);

    // Map the function f domain t0..t1 to the returned function domain 0..1
    // Conceptually t0 <= 0 < 1 <= t1 but this is not enforced
    template<typename T>
    std::function<T(const T&)> extend(const T& t0, const T& t1, std::function<T(const T&)> f);

    // Map the function f domain t0..t1 to the returned function domain 0..1
    // There is no implied relation between the range t0..t1 to the range 0..1
    template<typename T>
    std::function<T(const T&)> select(const T& t0, const T& t1, std::function<T(const T&)> f);

    // Map the function f domain t0..t0 + s to the returned function domain 0..1
    template<typename T>
    std::function<T(const T&)> domain(const T& t0, const T& s, std::function<T(const T&)> f);

    // Return t0 function with the range scaled
    template<typename T>
    std::function<T(const T&)> scale(const T& s, std::function<T(const T&)> f);

} // namespace quetzal::math

//------------------------------------------------------------------------------
template<typename T>
std::function<T(const T&)> quetzal::math::restrict(const T& t0, const T& t1, std::function<T(const T&)> f)
{
    return domain(t0, t1 - t0, f);
}

//------------------------------------------------------------------------------
template<typename T>
std::function<T(const T&)> quetzal::math::extend(const T& t0, const T& t1, std::function<T(const T&)> f)
{
    return domain(t0, t1 - t0, f);
}

//------------------------------------------------------------------------------
template<typename T>
std::function<T(const T&)> quetzal::math::select(const T& t0, const T& t1, std::function<T(const T&)> f)
{
    return domain(t0, t1 - t0, f);
}

//------------------------------------------------------------------------------
template<typename T>
std::function<T(const T&)> quetzal::math::domain(const T& t0, const T& s, std::function<T(const T&)> f)
{
    return [=](const T& t) -> T { return f(s * t + t0); };
}

//------------------------------------------------------------------------------
template<typename T>
std::function<T(const T&)> quetzal::math::scale(const T& s, std::function<T(const T&)> f)
{
    return [=](const T& t) -> T { return s * f(t); };
}

#endif // QUETZAL_MATH_PARAMETRIC_HPP
