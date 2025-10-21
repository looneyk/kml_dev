#if !defined(QUETZAL_MATH_MATRIX_UTIL_HPP)
#define QUETZAL_MATH_MATRIX_UTIL_HPP
//------------------------------------------------------------------------------
// math
// matrix_util.hpp
//------------------------------------------------------------------------------

#include "Matrix.hpp"
#include <functional>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>

namespace quetzal::math
{

    template<typename T, size_t M, size_t N>
    std::string to_formatted(const Matrix<T, M, N>& matrix, std::function<std::string(const T&)> formatter);

    template<typename T, size_t M, size_t N>
    std::string to_csv(const Matrix<T, M, N>& matrix, char separator = ',', bool quoted = false);

    template<typename T, size_t M, size_t N>
    std::string to_fixed_width(const Matrix<T, M, N>& matrix, int width = 24, int prec = std::numeric_limits<T>::max_digits10 - 1);

} // namespace quetzal::math

//------------------------------------------------------------------------------
template<typename T, size_t M, size_t N>
std::string quetzal::math::to_formatted(const Matrix<T, M, N>& matrix, std::function<std::string(const T&)> formatter)
{
    std::ostringstream oss(std::ostringstream::binary);

    auto data = matrix.data();
    size_t k = 0;
    for (size_t i = 0; i < M; ++i)
    {
        for (size_t j = 0; j < N; ++j)
        {
            if (j > 0)
            {
                oss << '\t';
            }

            oss << formatter(data[k++]);
        }

        oss << '\n';
    }

    return oss.str();
}

//------------------------------------------------------------------------------
template<typename T, size_t M, size_t N>
std::string quetzal::math::to_csv(const Matrix<T, M, N>& matrix, char separator, bool quoted)
{
    std::ostringstream oss(std::ostringstream::binary);
    oss << std::setprecision(std::numeric_limits<T>::max_digits10 - 1);

    auto data = matrix.data();
    size_t k = 0;
    for (size_t i = 0; i < M; ++i)
    {
        for (size_t j = 0; j < N; ++j)
        {
            if (j > 0)
            {
                oss << separator;
            }

            if (quoted)
            {
                oss << "\"";
            }
            oss << data[k++];
            if (quoted)
            {
                oss << "\"";
            }
        }

        oss << '\n';
    }

    return oss.str();
}

//------------------------------------------------------------------------------
template<typename T, size_t M, size_t N>
std::string quetzal::math::to_fixed_width(const Matrix<T, M, N>& matrix, int width, int prec)
{
    std::ostringstream oss(std::ostringstream::binary);

    auto data = matrix.data();
    size_t k = 0;
    for (size_t i = 0; i < M; ++i)
    {
        for (size_t j = 0; j < N; ++j)
        {
            std::ostringstream oss_temp;
            oss_temp << std::setprecision(prec) << data[k++];
            auto n = oss_temp.str().size();
            oss << std::string(width - n, ' ') << oss_temp.str();
        }

        oss << '\n';
    }

    return oss.str();
}

#endif // QUETZAL_MATH_MATRIX_UTIL_HPP
