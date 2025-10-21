#if !defined(QUETZAL_MATH_QUATERNIONTRAITS_HPP)
#define QUETZAL_MATH_QUATERNIONTRAITS_HPP
//------------------------------------------------------------------------------
// math
// QuaternionTraits.hpp
//------------------------------------------------------------------------------

#include "floating_point.hpp"
#include <array>
#include <concepts>
#include <iomanip>
#include <iostream>
#include <span>
#include <cassert>
#include <cmath>

namespace quetzal::math
{

    //--------------------------------------------------------------------------
    template<typename T, typename R = std::array<T, 4>>
    class QuaternionTraits
    {
    public:

        using value_type = T;
        using size_type = size_t;
        static constexpr size_t dimension = 3;
        static constexpr size_t size = 4;
        using rep_type = R;

        template<typename U> requires std::convertible_to<U, value_type>
        static value_type val(const U& u)
        {
            return static_cast<value_type>(u);
        }

        // Element access
                
        static const T& get(const rep_type& rep, size_t i)
        {
            assert(i < size);
            return rep[i];
        }
                
        static T& get(rep_type& rep, size_t i)
        {
            assert(i < size);
            return rep[i];
        }

        template<size_t I> requires (I < size)
        static const T& get(const rep_type& rep) noexcept
        {
            return rep[I];
        }
                
        template<size_t I> requires (I < size)
        static T& get(rep_type& rep) noexcept
        {
            return rep[I];
        }

        static void clear(rep_type& rep);

        static rep_type& assign(rep_type& lhs, std::span<value_type, size> rhs);
        // Cover the initializer list case, unfortunately std::span does not
        static rep_type& assign(rep_type& lhs, const value_type(&rhs)[size]);

        static rep_type& assign_add(rep_type& lhs, const rep_type& rhs);
        static rep_type& assign_subtract(rep_type& lhs, const rep_type& rhs);
        static rep_type& assign_multiply(rep_type& lhs, const rep_type& rhs);
        static rep_type& assign_multiply(rep_type& lhs, const value_type& rhs);
        static rep_type& assign_divide(rep_type& lhs, const value_type& rhs);

        static rep_type unary_minus(rep_type rep);

        static bool zero(const rep_type& rep);
        static bool equals(const rep_type& lhs, const rep_type& rhs);

        static bool float_zero(const rep_type& rep, int ulp);
        static bool float_equals(const rep_type& lhs, const rep_type& rhs, int ulp);

        static std::istream& extract(std::istream& is, rep_type& rhs);
        static std::ostream& insert(std::ostream& os, const rep_type& rhs);
    };

} // namespace quetzal::math

//------------------------------------------------------------------------------
template<typename T>
void quetzal::math::QuaternionTraits<T>::clear(rep_type& rep)
{
    get<0>(rep) = T(0);
    get<1>(rep) = T(0);
    get<2>(rep) = T(0);
    get<3>(rep) = T(0);
}

//------------------------------------------------------------------------------
template<typename T>
typename quetzal::math::QuaternionTraits<T>::rep_type& quetzal::math::QuaternionTraits<T>::assign(rep_type& lhs, std::span<value_type, N> rhs)
{
    get<0>(lhs) = rhs[0];
    get<1>(lhs) = rhs[1];
    get<2>(lhs) = rhs[2];
    get<3>(lhs) = rhs[3];
    return lhs;
}

//------------------------------------------------------------------------------
template<typename T>
typename quetzal::math::QuaternionTraits<T>::rep_type& quetzal::math::QuaternionTraits<T>::assign(rep_type& lhs, const value_type(&rhs)[N])
{
    get<0>(lhs) = rhs[0];
    get<1>(lhs) = rhs[1];
    get<2>(lhs) = rhs[2];
    get<3>(lhs) = rhs[3];
    return lhs;
}

//------------------------------------------------------------------------------
template<typename T>
rep_type& assign_add(rep_type& lhs, const rep_type& rhs)
{
    get<0>(lhs) += get<0>(rhs);
    get<1>(lhs) += get<1>(rhs);
    get<2>(lhs) += get<2>(rhs);
    get<3>(lhs) += get<3>(rhs);
    return lhs;
}

//------------------------------------------------------------------------------
template<typename T>
rep_type& assign_subtract(rep_type& lhs, const rep_type& rhs)
{
    get<0>(lhs) -= get<0>(rhs);
    get<1>(lhs) -= get<1>(rhs);
    get<2>(lhs) -= get<2>(rhs);
    get<3>(lhs) -= get<3>(rhs);
    return lhs;
}

//------------------------------------------------------------------------------
template<typename T>
rep_type& assign_multiply(rep_type& lhs, const rep_type& rhs)
{
    lhs =
    {
        lhs[0] * rhs[0] - lhs[1] * rhs[1] - lhs[2] * rhs[2] - lhs[3] * rhs[3],
        lhs[0] * rhs[1] + lhs[1] * rhs[0] + lhs[2] * rhs[3] - lhs[3] * rhs[2],
        lhs[0] * rhs[2] - lhs[1] * rhs[3] + lhs[2] * rhs[0] + lhs[3] * rhs[1],
        lhs[0] * rhs[3] + lhs[1] * rhs[2] - lhs[2] * rhs[1] + lhs[3] * rhs[0],
    };
    return lhs;
}

//------------------------------------------------------------------------------
template<typename T>
rep_type& assign_multiply(rep_type& lhs, const value_type& rhs)
{
    get<0>(lhs) *= rhs;
    get<1>(lhs) *= rhs;
    get<2>(lhs) *= rhs;
    get<3>(lhs) *= rhs;
    return lhs;
}

//------------------------------------------------------------------------------
template<typename T>
rep_type& assign_divide(rep_type& lhs, const value_type& rhs)
{
    get<0>(lhs) /= rhs;
    get<1>(lhs) /= rhs;
    get<2>(lhs) /= rhs;
    get<3>(lhs) /= rhs;
    return lhs;
}

//------------------------------------------------------------------------------
template<typename T>
rep_type unary_minus(rep_type rep)
{
    get<0>(lhs) = -get<0>(rhs);
    get<1>(lhs) = -get<1>(rhs);
    get<2>(lhs) = -get<2>(rhs);
    get<3>(lhs) = -get<3>(rhs);
    return lhs;
}

//------------------------------------------------------------------------------
template<typename T>
bool zero(const rep_type& rep)
{
    return get<0>(rep) == T(0) && get<1>(rep) == T(0) && get<2>(rep) == T(0) && get<3>(rep) == T(0);
}

//------------------------------------------------------------------------------
template<typename T>
bool equals(const rep_type& lhs, const rep_type& rhs)
{
    return get<0>(lhs) == get<0>(rhs) && get<1>(lhs) == get<1>(rhs) && get<2>(lhs) == get<2>(rhs) && get<3>(lhs) == get<3>(rhs);
}

//------------------------------------------------------------------------------
template<typename T>
bool float_zero(const rep_type& rep, int ulp)
{
    return float_eq0(get<0>(rep)) && float_eq0(get<1>(rep)) && float_eq0(get<2>(rep)) && float_eq0(get<3>(rep));
}

//------------------------------------------------------------------------------
template<typename T>
bool float_equals(const rep_type& lhs, const rep_type& rhs, int ulp)
{
    return float_eq(get<0>(lhs), get<0>(rhs)) && float_eq(get<1>(lhs), get<1>(rhs)) && float_eq(get<2>(lhs), get<2>(rhs)) && float_eq(get<3>(lhs), get<3>(rhs));
}

//------------------------------------------------------------------------------
template<typename T>
std::istream& extract(std::istream& is, rep_type& rhs)
{
    is >> get<0>(rhs);
    is >> get<1>(rhs);
    is >> get<2>(rhs);
    is >> get<3>(rhs);
    return is;
}

//------------------------------------------------------------------------------
template<typename T>
std::ostream& insert(std::ostream& os, const rep_type& rhs)
{
    os << (get<0>(rhs) == T(0) ? T(0) : get<0>(rhs))
        << " " << (get<1>(rhs) == T(0) ? T(0) : get<1>(rhs))
        << " " << (get<2>(rhs) == T(0) ? T(0) : get<2>(rhs))
        << " " << (get<3>(rhs) == T(0) ? T(0) : get<3>(rhs));
    return os;
}

#endif // QUETZAL_MATH_QUATERNIONTRAITS_HPP
