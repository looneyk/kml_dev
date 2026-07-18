#if !defined(QUETZAL_MATH_HOMOGENEOUSTRAITS_HPP)
#define QUETZAL_MATH_HOMOGENEOUSTRAITS_HPP
//------------------------------------------------------------------------------
// math
// HomogeneousTraits.hpp
//------------------------------------------------------------------------------

#include "floating_point.hpp"
#include "math_util.hpp"
#include <array>
#include <charconv>
#include <concepts>
#include <format>
#include <iomanip>
#include <iostream>
#include <limits>
#include <span>
#include <vector>
#include <cassert>
#include <cmath>

namespace quetzal::math
{

    //--------------------------------------------------------------------------
    template<typename T, size_t N>
    class HomogeneousTraits
    {
    public:

        using value_type = T;
        using size_type = size_t;
        static constexpr size_t dimension = N;
        static constexpr size_t size = N + 1;
        using rep_type = std::array<value_type, size>;

        template<size_t K>
        using resized_traits = HomogeneousTraits<value_type, K>;

        template<size_t S>
        static constexpr size_t effective_dimension = S - 1;

        template<size_t K>
        static constexpr bool accepts = (K == N || K == N + 1);

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

        template<size_t I = N> requires (I <= N)
        static void clear(rep_type& rep);

        template<size_t I = N - 1> requires (I < N)
        static rep_type& assign(rep_type& lhs, std::span<value_type, N> rhs);

        template<size_t I = N> requires (I <= N)
        static rep_type& assign(rep_type& lhs, std::span<value_type, N + 1> rhs);

        // Cover the initializer list case, unfortunately std::span does not
        template<size_t I = N - 1> requires (I < N)
        static rep_type& assign(rep_type& lhs, const value_type(&rhs)[N]);

        // Cover the initializer list case, unfortunately std::span does not
        template<size_t I = N> requires (I <= N)
        static rep_type& assign(rep_type& lhs, const value_type(&rhs)[N + 1]);

        // rhs should be vector (w == 0)
        template<size_t I = N> requires (I <= N)
        static rep_type& assign_add(rep_type& lhs, const rep_type& rhs);

        // rhs should be vector (w == 0) or both points (w == 1)
        template<size_t I = N> requires (I <= N)
        static rep_type& assign_subtract(rep_type& lhs, const rep_type& rhs);

        // Componentwise, allows for parallel operations across components
        template<size_t I = N> requires (I <= N)
        static rep_type& assign_multiply(rep_type& lhs, const rep_type& rhs);

        template<size_t I = N - 1> requires (I < N)
        static rep_type& assign_multiply(rep_type& lhs, const value_type& rhs);

        template<size_t I = N - 1> requires (I < N)
        static rep_type& assign_divide(rep_type& lhs, const value_type& rhs);

        // Only meaningful for vector (w == 0)
        template<size_t I = N - 1> requires (I < N)
        static rep_type unary_minus(rep_type rep);

        // Only applies to vectors (w == 0)
        template<size_t I = N - 1> requires (I < N)
        static T dot_product(const rep_type& lhs, const rep_type& rhs);

        template<size_t I = N - 1> requires (I < N)
        static bool valid(const rep_type& rep);

        template<size_t I = N - 1> requires (I < N)
        static bool zero(const rep_type& rep);

        template<size_t I = N - 1> requires (I < N)
        static bool equals(const rep_type& lhs, const rep_type& rhs);

        // Short-circuit || operation on components starting with highest dimension
        template<size_t I = N - 1> requires (I < N)
        static bool priority_less(const rep_type& lhs, const rep_type& rhs);

        // All components
        template<size_t I = N - 1> requires (I < N)
        static bool all_less(const rep_type& lhs, const rep_type& rhs);

        // Any component
        template<size_t I = N - 1> requires (I < N)
        static bool any_less(const rep_type& lhs, const rep_type& rhs);

        template<size_t I = N - 1> requires (I < N)
        static bool float_zero(const rep_type& rep, int tolerance);

        template<size_t I = N - 1> requires (I < N)
        static bool float_equals(const rep_type& lhs, const rep_type& rhs, int tolerance);

        // Short-circuit || operation on components starting with highest dimension
        template<size_t I = N - 1> requires (I < N)
        static bool float_priority_less(const rep_type& lhs, const rep_type& rhs, int tolerance);

        // Componentwise operations: denoise, abs, min, max

        template<size_t I = N> requires (I <= N)
        static void denoise0(rep_type& rep, int tolerance);

        template<size_t I = N> requires (I <= N)
        static void denoise01(rep_type& rep, int tolerance);

        template<size_t I = N> requires (I <= N)
        static void denoise011(rep_type& rep, int tolerance);

        template<size_t I = N - 1> requires (I < N)
        static void set_abs(rep_type& rep);

        template<size_t I = N - 1> requires (I < N)
        static void set_lowest(rep_type& rep);

        template<size_t I = N - 1> requires (I < N)
        static void set_highest(rep_type& rep);

        template<size_t I = N - 1> requires (I < N)
        static rep_type min(const rep_type& lhs, rep_type rhs);

        template<size_t I = N - 1> requires (I < N)
        static rep_type max(const rep_type& lhs, rep_type rhs);

        // For now, just checks that the pattern appears between elements with preceding and succeeding whitespace ignored ...
        // element_delimiter_pattern optional as well, and with object_delimiters should be inferred from parsing ...
        // this should just have two vertsions, one with known delimiters, and one with none that infers format ...
        // this should be (I < N), io should only read/write normalized values ...
        template<size_t I = N> requires (I <= N)
//        static std::istream& extract(std::istream& is, rep_type& rep, const std::string& element_delimiter_pattern = " *, *", const std::optional<std::pair<char, char>>& object_delimiters = std::make_optional(std::pair<char, char>{'(', ')'}));
        static std::istream& extract(std::istream& is, rep_type& rep, const std::string& element_delimiter_pattern = " **", const std::optional<std::pair<char, char>>& object_delimiters = std::nullopt);

        template<size_t I = N> requires (I <= N)
//        static std::ostream& insert(std::ostream& os, const rep_type& rep, const std::string& element_delimiter = ", ", const std::optional<std::pair<char, char>>& object_delimiters = std::make_optional(std::pair<char, char>{'(', ')'}));
        static std::ostream& insert(std::ostream& os, const rep_type& rep, const std::string& element_delimiter = " ", const std::optional<std::pair<char, char>>& object_delimiters = std::nullopt);

        template<size_t I = N - 1> requires (I < N)
        static std::from_chars_result from_chars(const char* first, const char* last, rep_type& rep);

        template<size_t I = N - 1> requires (I < N)
        static std::to_chars_result to_chars(char* first, char* last, const rep_type& rep, const std::string& element_delimiter = " ");

        template<typename FormatContext, size_t I = N - 1> requires (I < N)
        static typename FormatContext::iterator format_to(FormatContext& ctx, const std::formatter<T>& element_formatter, const rep_type& rep, const std::string& element_delimiter = " ", const std::optional<std::pair<char, char>>& object_delimiters = std::nullopt);

        static constexpr const T* data(const rep_type& rep) noexcept;
        static constexpr T* data(rep_type& rep) noexcept;

        // prepare: reduce, projective_divide, relative_projective_divide, ...
        // only applies to points, and so there is no explicit check for w != 0
        static void prepare(rep_type& rep);
        static void prepare(const rep_type& lhs, rep_type& rhs);

        template<size_t K>
        static void conclude(rep_type& rep, value_type w);

    private:
    
        // refactor these out ...
        std::istream& skip_object_delimiter(std::istream& is, char delimiter);
        std::istream& skip_element_delimiter(std::istream& is, const std::string& delimiter_pattern);

        static const char* skip_whitespace(const char* first, const char* last);

        // tolerance? should be exact? used where tolerance not available ...
        //static bool is_vector(const rep_type& rep, int tolerance = 64);

        // Reduction is projective division. In the unary and binary cases:
        // reduced means that the last component is 1 or that they are the same, respectively
        // reduce results in the last component being 1 or the same, respectively

        // these can be done directly in prepare ...

        static bool reduced(const rep_type& rep, int tolerance = 64);
        static bool reduced(const rep_type& lhs, const rep_type& rhs, int tolerance = 64);

        static void reduce(rep_type& rep);
        static void reduce(const rep_type& lhs, rep_type& rhs);

        // traits should not need to know anything about formatting, this should come in through parameters, not stored here ...
        // and extract would need to know what to parse for too ...
//        static constexpr std::string_view element_delimiters = " ,\t"; // regex for read, simple value for write, regex constructed? ...
//        static constexpr std::vector<std::pair<char, char>> object_delimiter_pairs = {{'(', ')'}, {'[', ']'}, {'{', '}'}, {'<', '>'}};
    };

} // namespace quetzal::math

//------------------------------------------------------------------------------
template<typename T, size_t N>
template<size_t I> requires (I <= N)
void quetzal::math::HomogeneousTraits<T, N>::clear(rep_type& rep)
{
    if constexpr (I == 0)
    {
        get<0>(rep) = T(0);
        return;
    }
    else
    {
        clear<I - 1>(rep);
        get<I>(rep) = T(0);
        return;
    }
}

//------------------------------------------------------------------------------
template<typename T, size_t N>
template<size_t I> requires (I < N)
typename quetzal::math::HomogeneousTraits<T, N>::rep_type& quetzal::math::HomogeneousTraits<T, N>::assign(rep_type& lhs, std::span<value_type, N> rhs)
{
    if constexpr (I == 0)
    {
        get<0>(lhs) = rhs[0];
        return lhs;
    }
    else
    {
        get<I>(lhs) = rhs[I];
        return assign<I - 1>(lhs, rhs);
    }
}

//------------------------------------------------------------------------------
template<typename T, size_t N>
template<size_t I> requires (I <= N)
typename quetzal::math::HomogeneousTraits<T, N>::rep_type& quetzal::math::HomogeneousTraits<T, N>::assign(rep_type& lhs, std::span<value_type, N + 1> rhs)
{
    if constexpr (I == 0)
    {
        get<0>(lhs) = rhs[0];
        return lhs;
    }
    else
    {
        get<I>(lhs) = rhs[I];
        return assign<I - 1>(lhs, rhs);
    }
}

//------------------------------------------------------------------------------
template<typename T, size_t N>
template<size_t I> requires (I < N)
typename quetzal::math::HomogeneousTraits<T, N>::rep_type& quetzal::math::HomogeneousTraits<T, N>::assign(rep_type& lhs, const value_type(&rhs)[N])
{
    if constexpr (I == 0)
    {
        get<0>(lhs) = rhs[0];
        return lhs;
    }
    else
    {
        get<I>(lhs) = rhs[I];
        return assign<I - 1>(lhs, rhs);
    }
}

//------------------------------------------------------------------------------
template<typename T, size_t N>
template<size_t I> requires (I <= N)
typename quetzal::math::HomogeneousTraits<T, N>::rep_type& quetzal::math::HomogeneousTraits<T, N>::assign(rep_type& lhs, const value_type(&rhs)[N + 1])
{
    if constexpr (I == 0)
    {
        get<0>(lhs) = rhs[0];
        return lhs;
    }
    else
    {
        get<I>(lhs) = rhs[I];
        return assign<I - 1>(lhs, rhs);
    }
}

//------------------------------------------------------------------------------
template<typename T, size_t N>
template<size_t I> requires (I <= N)
typename quetzal::math::HomogeneousTraits<T, N>::rep_type& quetzal::math::HomogeneousTraits<T, N>::assign_add(rep_type& lhs, const rep_type& rhs)
{
    if constexpr (I == 0)
    {
        get<0>(lhs) += get<0>(rhs);
        return lhs;
    }
    else
    {
        get<I>(lhs) += get<I>(rhs);
        return assign_add<I - 1>(lhs, rhs);
    }
}

//------------------------------------------------------------------------------
template<typename T, size_t N>
template<size_t I> requires (I <= N)
typename quetzal::math::HomogeneousTraits<T, N>::rep_type& quetzal::math::HomogeneousTraits<T, N>::assign_subtract(rep_type& lhs, const rep_type& rhs)
{
    if constexpr (I == 0)
    {
        get<0>(lhs) -= get<0>(rhs);
        return lhs;
    }
    else
    {
        get<I>(lhs) -= get<I>(rhs);
        return assign_subtract<I - 1>(lhs, rhs);
    }
}

//------------------------------------------------------------------------------
template<typename T, size_t N>
template<size_t I> requires (I <= N)
typename quetzal::math::HomogeneousTraits<T, N>::rep_type& quetzal::math::HomogeneousTraits<T, N>::assign_multiply(rep_type& lhs, const rep_type& rhs)
{
    if constexpr (I == 0)
    {
        get<0>(lhs) *= get<0>(rhs);
        return lhs;
    }
    else
    {
        get<I>(lhs) *= get<I>(rhs);
        return assign_multiply<I - 1>(lhs, rhs);
    }
}

//------------------------------------------------------------------------------
template<typename T, size_t N>
template<size_t I> requires (I < N)
typename quetzal::math::HomogeneousTraits<T, N>::rep_type& quetzal::math::HomogeneousTraits<T, N>::assign_multiply(rep_type& lhs, const value_type& rhs)
{
    if constexpr (I == 0)
    {
        get<0>(lhs) *= rhs;
        return lhs;
    }
    else
    {
        get<I>(lhs) *= rhs;
        return assign_multiply<I - 1>(lhs, rhs);
    }
}

//------------------------------------------------------------------------------
template<typename T, size_t N>
template<size_t I> requires (I < N)
typename quetzal::math::HomogeneousTraits<T, N>::rep_type& quetzal::math::HomogeneousTraits<T, N>::assign_divide(rep_type& lhs, const value_type& rhs)
{
    if constexpr (I == 0)
    {
        get<0>(lhs) /= rhs;
        return lhs;
    }
    else
    {
        get<I>(lhs) /= rhs;
        return assign_divide<I - 1>(lhs, rhs);
    }
}

//------------------------------------------------------------------------------
template<typename T, size_t N>
template<size_t I> requires (I < N)
typename quetzal::math::HomogeneousTraits<T, N>::rep_type quetzal::math::HomogeneousTraits<T, N>::unary_minus(rep_type rep)
{
    if constexpr (I == 0)
    {
        get<0>(rep) = -get<0>(rep);
        return rep;
    }
    else
    {
        get<I>(rep) = -get<I>(rep);
        return unary_minus<I - 1>(rep);
    }
}

//------------------------------------------------------------------------------
template<typename T, size_t N>
template<size_t I> requires (I < N)
T quetzal::math::HomogeneousTraits<T, N>::dot_product(const rep_type& lhs, const rep_type& rhs)
{
    if constexpr (I == 0)
    {
        return get<0>(lhs) * get<0>(rhs);
    }
    else
    {
        return get<I>(lhs) * get<I>(rhs) + dot_product<I - 1>(lhs, rhs);
//        return fma(get<I>(lhs), get<I>(rhs), dot_product<I - 1>(lhs, rhs));
    }
}

//------------------------------------------------------------------------------
template<typename T, size_t N>
template<size_t I> requires (I < N)
bool quetzal::math::HomogeneousTraits<T, N>::valid(const rep_type& rep)
{
    if constexpr (I == 0)
    {
        return math::valid(get<0>(rep));
    }
    else
    {
        if (math::valid(get<I>(rep)))
        {
            return valid<I - 1>(rep);
        }

        return false;
    }
}

//------------------------------------------------------------------------------
template<typename T, size_t N>
template<size_t I> requires (I < N)
bool quetzal::math::HomogeneousTraits<T, N>::zero(const rep_type& rep)
{
    if constexpr (I == 0)
    {
        return get<0>(rep) == T(0);
    }
    else
    {
        return get<I>(rep) == T(0) && zero<I - 1>(rep);
    }
}

//------------------------------------------------------------------------------
template<typename T, size_t N>
template<size_t I> requires (I < N)
bool quetzal::math::HomogeneousTraits<T, N>::equals(const rep_type& lhs, const rep_type& rhs)
{
    if constexpr (I == 0)
    {
        return get<0>(lhs) == get<0>(rhs);
    }
    else
    {
        return get<I>(lhs) == get<I>(rhs) && equals<I - 1>(lhs, rhs);
    }
}

//------------------------------------------------------------------------------
template<typename T, size_t N>
template<size_t I> requires (I < N)
bool quetzal::math::HomogeneousTraits<T, N>::priority_less(const rep_type& lhs, const rep_type& rhs)
{
    if constexpr (I == 0)
    {
        return get<0>(lhs) < get<0>(rhs);
    }
    else
    {
        if (get<I>(lhs) == get<I>(rhs))
        {
            return priority_less<I - 1>(lhs, rhs);
        }

        return get<I>(lhs) < get<I>(rhs);
    }
}

//------------------------------------------------------------------------------
template<typename T, size_t N>
template<size_t I> requires (I < N)
bool quetzal::math::HomogeneousTraits<T, N>::all_less(const rep_type& lhs, const rep_type& rhs)
{
    if constexpr (I == 0)
    {
        return get<0>(lhs) < get<0>(rhs);
    }
    else
    {
        return get<I>(lhs) < get<I>(rhs) && all_less<I - 1>(lhs, rhs);
    }
}

//------------------------------------------------------------------------------
template<typename T, size_t N>
template<size_t I> requires (I < N)
bool quetzal::math::HomogeneousTraits<T, N>::any_less(const rep_type& lhs, const rep_type& rhs)
{
    if constexpr (I == 0)
    {
        return get<0>(lhs) < get<0>(rhs);
    }
    else
    {
        return get<I>(lhs) < get<I>(rhs) || any_less<I - 1>(lhs, rhs);
    }
}

//------------------------------------------------------------------------------
template<typename T, size_t N>
template<size_t I> requires (I < N)
bool quetzal::math::HomogeneousTraits<T, N>::float_zero(const rep_type& rep, int tolerance)
{
    if constexpr (I == 0)
    {
        return quetzal::math::float_eq0(get<0>(rep), tolerance);
    }
    else
    {
        return quetzal::math::float_eq0(get<I>(rep), tolerance) && float_zero<I - 1>(rep, tolerance);
    }
}

//------------------------------------------------------------------------------
template<typename T, size_t N>
template<size_t I> requires (I < N)
bool quetzal::math::HomogeneousTraits<T, N>::float_equals(const rep_type& lhs, const rep_type& rhs, int tolerance)
{
    if constexpr (I == 0)
    {
        return quetzal::math::float_eq(get<0>(lhs), get<0>(rhs), tolerance);
    }
    else
    {
        return quetzal::math::float_eq(get<I>(lhs), get<I>(rhs), tolerance) && float_equals<I - 1>(lhs, rhs, tolerance);
    }
}

//------------------------------------------------------------------------------
template<typename T, size_t N>
template<size_t I> requires (I < N)
bool quetzal::math::HomogeneousTraits<T, N>::float_priority_less(const rep_type& lhs, const rep_type& rhs, int tolerance)
{
    if constexpr (I == 0)
    {
        return float_lt(get<0>(lhs), get<0>(rhs), tolerance);
    }
    else
    {
        if (float_eq(get<I>(lhs), get<I>(rhs), tolerance))
        {
            return priority_less<I - 1>(lhs, rhs);
        }

        return float_lt(get<I>(lhs), get<I>(rhs), tolerance);
    }
}

//------------------------------------------------------------------------------
template<typename T, size_t N>
template<size_t I> requires (I <= N)
void quetzal::math::HomogeneousTraits<T, N>::denoise0(rep_type& rep, int tolerance)
{
    if constexpr (I == 0)
    {
        get<0>(rep) = math::denoise0(get<0>(rep), tolerance);
        return;
    }
    else
    {
        denoise0<I - 1>(rep, tolerance);
        get<I>(rep) = math::denoise0(get<I>(rep), tolerance);
        return;
    }
}

//------------------------------------------------------------------------------
template<typename T, size_t N>
template<size_t I> requires (I <= N)
void quetzal::math::HomogeneousTraits<T, N>::denoise01(rep_type& rep, int tolerance)
{
    if constexpr (I == 0)
    {
        get<0>(rep) = math::denoise01(get<0>(rep), tolerance);
        return;
    }
    else
    {
        denoise01<I - 1>(rep, tolerance);
        get<I>(rep) = math::denoise01(get<I>(rep), tolerance);
        return;
    }
}

//------------------------------------------------------------------------------
template<typename T, size_t N>
template<size_t I> requires (I <= N)
void quetzal::math::HomogeneousTraits<T, N>::denoise011(rep_type& rep, int tolerance)
{
    if constexpr (I == 0)
    {
        get<0>(rep) = math::denoise011(get<0>(rep), tolerance);
        return;
    }
    else
    {
        denoise01<I - 1>(rep, tolerance);
        get<I>(rep) = math::denoise011(get<I>(rep), tolerance);
        return;
    }
}

//------------------------------------------------------------------------------
template<typename T, size_t N>
template<size_t I> requires (I < N)
void quetzal::math::HomogeneousTraits<T, N>::set_abs(rep_type& rep)
{
    if constexpr (I == 0)
    {
        get<0>(rep) = abs(get<0>(rep));
        return;
    }
    else
    {
        set_abs<I - 1>(rep);
        get<I>(rep) = abs(get<I>(rep));
        return;
    }
}

//------------------------------------------------------------------------------
template<typename T, size_t N>
template<size_t I> requires (I < N)
void quetzal::math::HomogeneousTraits<T, N>::set_lowest(rep_type& rep)
{
    if constexpr (I == 0)
    {
        get<0>(rep) = std::numeric_limits<T>::lowest();
        return;
    }
    else
    {
        set_lowest<I - 1>(rep);
        get<I>(rep) = std::numeric_limits<T>::lowest();
        return;
    }
}

//------------------------------------------------------------------------------
template<typename T, size_t N>
template<size_t I> requires (I < N)
void quetzal::math::HomogeneousTraits<T, N>::set_highest(rep_type& rep)
{
    if constexpr (I == 0)
    {
        get<0>(rep) = std::numeric_limits<T>::max();
        return;
    }
    else
    {
        set_highest<I - 1>(rep);
        get<I>(rep) = std::numeric_limits<T>::max();
        return;
    }
}

//------------------------------------------------------------------------------
template<typename T, size_t N>
template<size_t I> requires (I < N)
typename quetzal::math::HomogeneousTraits<T, N>::rep_type quetzal::math::HomogeneousTraits<T, N>::min(const rep_type& lhs, rep_type rhs)
{
    if constexpr (I == 0)
    {
        get<0>(rhs) = std::min(get<0>(lhs), get<0>(rhs));
        return rhs;
    }
    else
    {
        get<I>(rhs) = std::min(get<I>(lhs), get<I>(rhs));
        return min<I - 1>(lhs, rhs);
    }
}

//------------------------------------------------------------------------------
template<typename T, size_t N>
template<size_t I> requires (I < N)
typename quetzal::math::HomogeneousTraits<T, N>::rep_type quetzal::math::HomogeneousTraits<T, N>::max(const rep_type& lhs, rep_type rhs)
{
    if constexpr (I == 0)
    {
        get<0>(rhs) = std::max(get<0>(lhs), get<0>(rhs));
        return rhs;
    }
    else
    {
        get<I>(rhs) = std::max(get<I>(lhs), get<I>(rhs));
        return max<I - 1>(lhs, rhs);
    }
}

//------------------------------------------------------------------------------
template<typename T, size_t N>
template<size_t I> requires (I <= N)
std::istream& quetzal::math::HomogeneousTraits<T, N>::extract(std::istream& is, rep_type& rep, const std::string& element_delimiter_pattern, const std::optional<std::pair<char, char>>& object_delimiters)
{
    if constexpr (I == 0)
    {
        if (skip_object_delimiter(is, object_delimiters->first))
        {
            is >> get<0>(rep);
        }
        return is;
    }
    else
    {
        extract<I - 1>(is, rep);
        if (skip_element_delimiter(is, element_delimiter_pattern))
        {
            is >> get<I>(rep);
            if (I == N - 1)
            {
                skip_object_delimiter(is, object_delimiters->second);
            }
        }
        return is;
    }
}

//------------------------------------------------------------------------------
template<typename T, size_t N>
template<size_t I> requires (I <= N)
std::ostream& quetzal::math::HomogeneousTraits<T, N>::insert(std::ostream& os, const rep_type& rep, const std::string& element_delimiter, const std::optional<std::pair<char, char>>& object_delimiters)
{
    // Conditional expression ensures that -0 displays as 0

    if constexpr (I == 0)
    {
        if (object_delimiters) os << object_delimiters->first;
        os << (get<0>(rep) == T(0) ? T(0) : get<0>(rep));
        return os;
    }
    else
    {
        std::ios state(nullptr);
        if constexpr (I == N)
        {
            state.copyfmt(os);
        }

        insert<I - 1>(os, rep, element_delimiter, object_delimiters);
        os << element_delimiter << (get<I>(rep) == T(0) ? T(0) : get<I>(rep));
        if constexpr (I == N - 1)
        {
            if (object_delimiters) os << object_delimiters->second;
        }

        if constexpr (I == N)
        {
            os.copyfmt(state);
        }

        return os;
    }
}

//------------------------------------------------------------------------------
template<typename T, size_t N>
template<size_t I> requires (I < N)
std::from_chars_result quetzal::math::HomogeneousTraits<T, N>::from_chars(const char* first, const char* last, rep_type& rep)
{
    if constexpr (I == 0)
    {
        return std::from_chars(skip_whitespace(first, last), last, get<0>(rep));
    }
    else
    {
        auto result = from_chars<I - 1>(first, last, rep);
//        if (!result) ...
        if (result.ec != std::errc{})
        {
            return result;
        }

        return std::from_chars(skip_whitespace(result.ptr, last), last, get<I>(rep));
    }
}

//------------------------------------------------------------------------------
template<typename T, size_t N>
template<size_t I> requires (I < N)
std::to_chars_result quetzal::math::HomogeneousTraits<T, N>::to_chars(char* first, char* last, const rep_type& rep, const std::string& element_delimiter)
{
    // Conditional expression ensures that -0 displays as 0

    if constexpr (I == 0)
    {
        return std::to_chars(first, last, get<0>(rep) == T(0) ? T(0) : get<0>(rep));
    }
    else
    {
        auto result = to_chars<I - 1>(first, last, rep, element_delimiter);
        if (result.ec != std::errc{})
        {
            return result;
        }

        char* p = result.ptr;
        if (static_cast<size_t>(last - p) < element_delimiter.size())
        {
            return {p, std::errc::value_too_large};
        }

        for (char c : element_delimiter)
        {
            *p++ = c;
        }

        return std::to_chars(p, last, get<I>(rep) == T(0) ? T(0) : get<I>(rep));
    }
}

//------------------------------------------------------------------------------
template<typename T, size_t N>
template<typename FormatContext, size_t I> requires (I < N)
typename FormatContext::iterator quetzal::math::HomogeneousTraits<T, N>::format_to(FormatContext& ctx, const std::formatter<T>& element_formatter, const rep_type& rep, const std::string& element_delimiter, const std::optional<std::pair<char, char>>& object_delimiters)
{
    // Conditional expression ensures that -0 displays as 0

    if constexpr (I == 0)
    {
        if (object_delimiters)
        {
            auto out = ctx.out();
            *out++ = object_delimiters->first;
            ctx.advance_to(out);
        }

        return element_formatter.format(get<0>(rep) == T(0) ? T(0) : get<0>(rep), ctx);
    }
    else
    {
        auto out = format_to<FormatContext, I - 1>(ctx, element_formatter, rep, element_delimiter, object_delimiters);
        ctx.advance_to(out);
        for (char c : element_delimiter)
        {
            *out++ = c;
        }

        ctx.advance_to(out);
        out = element_formatter.format(get<I>(rep) == T(0) ? T(0) : get<I>(rep), ctx);

        if constexpr (I == N - 1)
        {
            if (object_delimiters)
            {
                ctx.advance_to(out);
                *out++ = object_delimiters->second;
            }
        }

        return out;
    }
}

//------------------------------------------------------------------------------
template<typename T, size_t N>
constexpr const T* quetzal::math::HomogeneousTraits<T, N>::data(const rep_type& rep) noexcept
{
    return rep.data();
}

//------------------------------------------------------------------------------
template<typename T, size_t N>
constexpr T* quetzal::math::HomogeneousTraits<T, N>::data(rep_type& rep) noexcept
{
    return rep.data();
}

//------------------------------------------------------------------------------
template<typename T, size_t N>
void quetzal::math::HomogeneousTraits<T, N>::prepare(rep_type& rep)
{
    if (!reduced(rep))
    {
        reduce(rep);
    }

    return;
}

//------------------------------------------------------------------------------
template<typename T, size_t N>
void quetzal::math::HomogeneousTraits<T, N>::prepare(const rep_type& lhs, rep_type& rhs)
{
    if (!reduced(lhs, rhs))
    {
        reduce(lhs, rhs);
    }

    return;
}

//------------------------------------------------------------------------------
template<typename T, size_t N>
template<size_t K>
void quetzal::math::HomogeneousTraits<T, N>::conclude(rep_type& rep, value_type w)
{
    if constexpr (K <= N)
    {
        get<N>(rep) = w;
    }

    return;
}

//------------------------------------------------------------------------------
/*
template<typename T, size_t N>
bool quetzal::math::HomogeneousTraits<T, N>::is_vector(const rep_type& rep, int tolerance)
{
    return quetzal::math::float_eq0(get<N>(rep), tolerance);
}
*/

//------------------------------------------------------------------------------
template<typename T, size_t N>
bool quetzal::math::HomogeneousTraits<T, N>::reduced(const rep_type& rep, int tolerance)
{
    return quetzal::math::float_eq(get<N>(rep), T(1), tolerance);
}

//------------------------------------------------------------------------------
template<typename T, size_t N>
bool quetzal::math::HomogeneousTraits<T, N>::reduced(const rep_type& lhs, const rep_type& rhs, int tolerance)
{
    return quetzal::math::float_eq(get<N>(lhs), get<N>(rhs), tolerance);
}

//------------------------------------------------------------------------------
template<typename T, size_t N>
void quetzal::math::HomogeneousTraits<T, N>::reduce(rep_type& rep)
{
    assign_divide(rep, get<N>(rep));
    conclude<N>(rep, T(1)); // exact
    return;
}

//------------------------------------------------------------------------------
template<typename T, size_t N>
void quetzal::math::HomogeneousTraits<T, N>::reduce(const rep_type& lhs, rep_type& rhs)
{
    assign_divide(assign_multiply(rhs, get<N>(lhs)), get<N>(rhs));
    conclude<N>(rhs, get<N>(lhs)); // exact
    return;
}

//------------------------------------------------------------------------------
template<typename T, size_t N>
std::istream& quetzal::math::HomogeneousTraits<T, N>::skip_object_delimiter(std::istream& is, char delimiter)
{
    is.skipws();
    char c = is.peek();
    if (c == delimiter)
    {
        is.ignore();
    }
    else
    {
        is.setstate(std::ios::failbit);
    }
    return is;
}

//------------------------------------------------------------------------------
template<typename T, size_t N>
std::istream& quetzal::math::HomogeneousTraits<T, N>::skip_element_delimiter(std::istream& is, const std::string& delimiter_pattern)
{
    // shortcut for now, probably fine forever ...
    is.skipws();
    if (delimiter_pattern.empty())
    {
        return is;
    }
    // for (each char in delimiter_pattern) ...
    char c = is.peek();
    if (c == delimiter_pattern[0]) // wrong, check each pattern section in sequence ...
    {
        is.ignore();
    }
    else
    {
        is.setstate(std::ios::failbit);
    }
    return is;
}

//------------------------------------------------------------------------------
template<typename T, size_t N>
const char* quetzal::math::HomogeneousTraits<T, N>::skip_whitespace(const char* first, const char* last)
{
    while (first != last && (*first == ' ' || *first == '\t'))
    {
        ++first;
    }
    return first;
}

#endif // QUETZAL_MATH_HOMOGENEOUSTRAITS_HPP
