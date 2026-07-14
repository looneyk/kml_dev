#if !defined(QUETZAL_MATH_SPHERICAL_HARMONICS_HPP)
#define QUETZAL_MATH_SPHERICAL_HARMONICS_HPP
//------------------------------------------------------------------------------
// brep
// mesh_operations.hpp
//------------------------------------------------------------------------------

#include <complex>
#include <functional>
#include <numbers>
#include <cassert>

namespace quetzal::math
{

    //--------------------------------------------------------------------------
    // degree l, order m
    // Returns a function taking the precomputed cos and sin of standard parameters, (theta, phi) in radians where theta is the polar angle
    // Note that the quetzal/model convention (phi, theta) uses turns where theta is the elevation angle
    template<typename T = double>
    std::function<std::complex<T>(T, T)> spherical_harmonic_function(int degree, int order);

    template<typename T = double>
    std::function<T(T, T)> real_spherical_harmonic_function(int degree, int order);

namespace detail
{

    template<typename T>
    std::function<T(T)> normalization_polar_component(int degree, int order);

    template<typename T>
    std::function<T(T)> real_azimuthal_component(int order);

    template<typename T>
    std::function<T(T)> imaginary_azimuthal_component(int order);

} // namespace detail

} // namespace quetzal::math

//------------------------------------------------------------------------------
template<typename T>
std::function<std::complex<T>(T, T)> quetzal::math::spherical_harmonic_function(int degree, int order)
{
    assert(degree >= 0);
    assert(std::abs(order) <= degree);

    return [degree, order](T theta, T phi) -> T
    {
        auto normalization_polar = detail::normalization_polar_component<T>(degree, order)(theta);
        return [&](T phi) -> std::complex<T> {
            return normalization_polar * detail::real_azimuthal_component<T>(order)(phi), normalization_polar * detail::imaginary_azimuthal_component<T>(order)(phi);
        };
    };
}

//------------------------------------------------------------------------------
template<typename T>
std::function<T(T, T)> quetzal::math::real_spherical_harmonic_function(int degree, int order)
{
    assert(degree >= 0);
    assert(std::abs(order) <= degree);

    return [degree, order](T theta, T phi) -> T
    {
        return detail::normalization_polar_component<T>(degree, order)(theta) * detail::real_azimuthal_component<T>(order)(phi);
    };
}

//------------------------------------------------------------------------------
template<typename T>
std::function<T(T)> quetzal::math::detail::normalization_polar_component(int degree, int order)
{
    auto PI = std::numbers::pi_v<T>;

    switch (degree)
    {
        case 0:
            switch (order)
            {
                case 0:
                    return [PI](T) -> T { return (1.0 / 2.0) * sqrt(1.0 / PI); };
            }
            break;

        case 1:
            switch (order)
            {
                case -1:
                    return [PI](T theta) -> T { return (1.0 / 2.0) * sqrt(3.0 / (2.0 * PI)) * sin(theta); };
                case 0:
                    return [PI](T theta) -> T { return (1.0 / 2.0) * sqrt(3.0 / PI) * cos(theta); };
                case 1:
                    return [PI](T theta) -> T { return -(1.0 / 2.0) * sqrt(3.0 / (2.0 * PI)) * sin(theta); };
            }
            break;

        case 2:
            switch (order)
            {
                case -2:
                    return [PI](T theta) -> T { return (1.0 / 4.0) * sqrt(15.0 / (2.0 * PI)) * pow(sin(theta), 2); };
                case -1:
                    return [PI](T theta) -> T { return (1.0 / 2.0) * sqrt(15.0 / (2.0 * PI)) * sin(theta) * cos(theta); };
                case 0:
                    return [PI](T theta) -> T { return (1.0 / 4.0) * sqrt(5.0 / PI) * (3.0 * pow(cos(theta), 2) - 1.0); };
                case 1:
                    return [PI](T theta) -> T { return -(1.0 / 2.0) * sqrt(15.0 / (2.0 * PI)) * sin(theta) * cos(theta); };
                case 2:
                    return [PI](T theta) -> T { return (1.0 / 4.0) * sqrt(15.0 / (2.0 * PI)) * pow(sin(theta), 2); };
            }
            break;

        case 3:
            switch (order)
            {
                case -3:
                    return [PI](T theta) -> T { return (1.0 / 8.0) * sqrt(35.0 / PI) * pow(sin(theta), 3); };
                case -2:
                    return [PI](T theta) -> T { return (1.0 / 4.0) * sqrt(105.0 / (2.0 * PI)) * pow(sin(theta), 2) * cos(theta); };
                case -1:
                    return [PI](T theta) -> T { return (1.0 / 8.0) * sqrt(21.0 / PI) * sin(theta) * (5.0 * pow(cos(theta), 2) - 1.0); };
                case 0:
                    return [PI](T theta) -> T { return (1.0 / 4.0) * sqrt(7.0 / PI) * (5.0 * pow(cos(theta), 3) - 3.0 * cos(theta)); };
                case 1:
                    return [PI](T theta) -> T { return -(1.0 / 8.0) * sqrt(21.0 / PI) * sin(theta) * (5.0 * pow(cos(theta), 2) - 1.0); };
                case 2:
                    return [PI](T theta) -> T { return (1.0 / 4.0) * sqrt(105.0 / (2.0 * PI)) * pow(sin(theta), 2) * cos(theta); };
                case 3:
                    return [PI](T theta) -> T { return -(1.0 / 8.0) * sqrt(35.0 / PI) * pow(sin(theta), 3); };
            }
            break;

        case 4:
            switch (order)
            {
                case -4:
                    return [PI](T theta) -> T { return (3.0 / 16.0) * sqrt(35.0 / (2.0 * PI)) * pow(sin(theta), 4); };
                case -3:
                    return [PI](T theta) -> T { return (3.0 / 8.0) * sqrt(35.0 / PI) * pow(sin(theta), 3) * cos(theta); };
                case -2:
                    return [PI](T theta) -> T { return (3.0 / 8.0) * sqrt(5.0 / (2.0 * PI)) * pow(sin(theta), 2) * (7.0 * pow(cos(theta), 2) - 1.0); };
                case -1:
                    return [PI](T theta) -> T { return (3.0 / 8.0) * sqrt(5.0 / PI) * sin(theta) * (7.0 * pow(cos(theta), 3) - 3.0 * cos(theta)); };
                case 0:
                    return [PI](T theta) -> T { return (3.0 / 16.0) * sqrt(1.0 / PI) * (35.0 * pow(cos(theta), 4) - 30.0 * pow(cos(theta), 2) + 3.0); };
                case 1:
                    return [PI](T theta) -> T { return -(3.0 / 8.0) * sqrt(5.0 / PI) * sin(theta) * (7.0 * pow(cos(theta), 3) - 3.0 * cos(theta)); };
                case 2:
                    return [PI](T theta) -> T { return (3.0 / 8.0) * sqrt(5.0 / (2.0 * PI)) * pow(sin(theta), 2) * (7.0 * pow(cos(theta), 2) - 1.0); };
                case 3:
                    return [PI](T theta) -> T { return -(3.0 / 8.0) * sqrt(35.0 / PI) * pow(sin(theta), 3) * cos(theta); };
                case 4:
                    return [PI](T theta) -> T { return (3.0 / 16.0) * sqrt(35.0 / (2.0 * PI)) * pow(sin(theta), 4); };
            }
            break;

        case 5:
            switch (order)
            {
                case -5:
                    return [PI](T theta) -> T { return (3.0 / 32.0) * sqrt(77.0 / PI) * pow(sin(theta), 5); };
                case -4:
                    return [PI](T theta) -> T { return (3.0 / 16.0) * sqrt(385.0 / (2.0 * PI)) * pow(sin(theta), 4) * cos(theta); };
                case -3:
                    return [PI](T theta) -> T { return (1.0 / 32.0) * sqrt(385.0 / PI) * pow(sin(theta), 3) * (9.0 * pow(cos(theta), 2) - 1.0); };
                case -2:
                    return [PI](T theta) -> T { return (1.0 / 8.0) * sqrt(1155.0 / (2.0 * PI)) * pow(sin(theta), 2) * (3.0 * pow(cos(theta), 3) - cos(theta)); };
                case -1:
                    return [PI](T theta) -> T { return (1.0 / 16.0) * sqrt(165.0 / (2.0 * PI)) * sin(theta) * (21.0 * pow(cos(theta), 4) - 14.0 * pow(cos(theta), 2) + 1.0); };
                case 0:
                    return [PI](T theta) -> T { return (1.0 / 16.0) * sqrt(11.0 / PI) * (63.0 * pow(cos(theta), 5) - 70.0 * pow(cos(theta), 3) + 15.0 * cos(theta)); };
                case 1:
                    return [PI](T theta) -> T { return -(1.0 / 16.0) * sqrt(165.0 / (2.0 * PI)) * sin(theta) * (21.0 * pow(cos(theta), 4) - 14.0 * pow(cos(theta), 2) + 1.0); };
                case 2:
                    return [PI](T theta) -> T { return (1.0 / 8.0) * sqrt(1155.0 / (2.0 * PI)) * pow(sin(theta), 2) * (3.0 * pow(cos(theta), 3) - cos(theta)); };
                case 3:
                    return [PI](T theta) -> T { return -(1.0 / 32.0) * sqrt(385.0 / PI) * pow(sin(theta), 3) * (9.0 * pow(cos(theta), 2) - 1.0); };
                case 4:
                    return [PI](T theta) -> T { return (3.0 / 16.0) * sqrt(385.0 / (2.0 * PI)) * pow(sin(theta), 4) * cos(theta); };
                case 5:
                    return [PI](T theta) -> T { return -(3.0 / 32.0) * sqrt(77.0 / PI) * pow(sin(theta), 5); };
            }
            break;

        case 6:
            switch (order)
            {
                case -6:
                    return [PI](T theta) -> T { return (1.0 / 64.0) * sqrt(3003.0 / PI) * pow(sin(theta), 6); };
                case -5:
                    return [PI](T theta) -> T { return (3.0 / 32.0) * sqrt(1001.0 / PI) * pow(sin(theta), 5) * cos(theta); };
                case -4:
                    return [PI](T theta) -> T { return (3.0 / 32.0) * sqrt(91.0 / (2.0 * PI)) * pow(sin(theta), 4) * (11.0 * pow(cos(theta), 2) - 1.0); };
                case -3:
                    return [PI](T theta) -> T { return (1.0 / 32.0) * sqrt(1365.0 / PI) * pow(sin(theta), 3) * (11.0 * pow(cos(theta), 3) - 3.0 * cos(theta)); };
                case -2:
                    return [PI](T theta) -> T { return (1.0 / 64.0) * sqrt(1365.0 / PI) * pow(sin(theta), 2) * (33.0 * pow(cos(theta), 4) - 18.0 * pow(cos(theta), 2) + 1.0); };
                case -1:
                    return [PI](T theta) -> T { return (1.0 / 16.0) * sqrt(273.0 / (2.0 * PI)) * sin(theta) * (33.0 * pow(cos(theta), 5) - 30.0 * pow(cos(theta), 3) + 5.0 * cos(theta)); };
                case 0:
                    return [PI](T theta) -> T { return (1.0 / 32.0) * sqrt(13.0 / PI) * (231.0 * pow(cos(theta), 6) - 315.0 * pow(cos(theta), 4) + 105.0 * pow(cos(theta), 2) - 5.0); };
                case 1:
                    return [PI](T theta) -> T { return -(1.0 / 16.0) * sqrt(273.0 / (2.0 * PI)) * sin(theta) * (33.0 * pow(cos(theta), 5) - 30.0 * pow(cos(theta), 3) + 5.0 * cos(theta)); };
                case 2:
                    return [PI](T theta) -> T { return (1.0 / 64.0) * sqrt(1365.0 / PI) * pow(sin(theta), 2) * (33.0 * pow(cos(theta), 4) - 18.0 * pow(cos(theta), 2) + 1.0); };
                case 3:
                    return [PI](T theta) -> T { return -(1.0 / 32.0) * sqrt(1365.0 / PI) * pow(sin(theta), 3) * (11.0 * pow(cos(theta), 3) - 3.0 * cos(theta)); };
                case 4:
                    return [PI](T theta) -> T { return (3.0 / 32.0) * sqrt(91.0 / (2.0 * PI)) * pow(sin(theta), 4) * (11.0 * pow(cos(theta), 2) - 1.0); };
                case 5:
                    return [PI](T theta) -> T { return -(3.0 / 32.0) * sqrt(1001.0 / PI) * pow(sin(theta), 5) * cos(theta); };
                case 6:
                    return [PI](T theta) -> T { return (1.0 / 64.0) * sqrt(3003.0 / PI) * pow(sin(theta), 6); };
            }
            break;

        case 7:
            switch (order)
            {
                case -7:
                    return [PI](T theta) -> T { return (3.0 / 64.0) * sqrt(715.0 / (2.0 * PI)) * pow(sin(theta), 7); };
                case -6:
                    return [PI](T theta) -> T { return (3.0 / 64.0) * sqrt(5005.0 / PI) * pow(sin(theta), 6) * cos(theta); };
                case -5:
                    return [PI](T theta) -> T { return (3.0 / 64.0) * sqrt(385.0 / (2.0 * PI)) * pow(sin(theta), 5) * (13.0 * pow(cos(theta), 2) - 1.0); };
                case -4:
                    return [PI](T theta) -> T { return (3.0 / 32.0) * sqrt(385.0 / (2.0 * PI)) * pow(sin(theta), 4) * (13.0 * pow(cos(theta), 3) - 3.0 * cos(theta)); };
                case -3:
                    return [PI](T theta) -> T { return (3.0 / 64.0) * sqrt(35.0 / (2.0 * PI)) * pow(sin(theta), 3) * (143.0 * pow(cos(theta), 4) - 66.0 * pow(cos(theta), 2) + 3.0); };
                case -2:
                    return [PI](T theta) -> T { return (3.0 / 64.0) * sqrt(35.0 / PI) * pow(sin(theta), 2) * (143.0 * pow(cos(theta), 5) - 110.0 * pow(cos(theta), 3) + 15.0 * cos(theta)); };
                case -1:
                    return [PI](T theta) -> T { return (1.0 / 64.0) * sqrt(105.0 / (2.0 * PI)) * sin(theta) * (429.0 * pow(cos(theta), 6) - 495.0 * pow(cos(theta), 4) + 135.0 * pow(cos(theta), 2) - 5.0); };
                case 0:
                    return [PI](T theta) -> T { return (1.0 / 32.0) * sqrt(15.0 / PI) * (429.0 * pow(cos(theta), 7) - 693.0 * pow(cos(theta), 5) + 315.0 * pow(cos(theta), 3) - 35.0 * cos(theta)); };
                case 1:
                    return [PI](T theta) -> T { return -(1.0 / 64.0) * sqrt(105.0 / (2.0 * PI)) * sin(theta) * (429.0 * pow(cos(theta), 6) - 495.0 * pow(cos(theta), 4) + 135.0 * pow(cos(theta), 2) - 5.0); };
                case 2:
                    return [PI](T theta) -> T { return (3.0 / 64.0) * sqrt(35.0 / PI) * pow(sin(theta), 2) * (143.0 * pow(cos(theta), 5) - 110.0 * pow(cos(theta), 3) + 15.0 * cos(theta)); };
                case 3:
                    return [PI](T theta) -> T { return -(3.0 / 64.0) * sqrt(35.0 / (2.0 * PI)) * pow(sin(theta), 3) * (143.0 * pow(cos(theta), 4) - 66.0 * pow(cos(theta), 2) + 3.0); };
                case 4:
                    return [PI](T theta) -> T { return (3.0 / 32.0) * sqrt(385.0 / (2.0 * PI)) * pow(sin(theta), 4) * (13.0 * pow(cos(theta), 3) - 3.0 * cos(theta)); };
                case 5:
                    return [PI](T theta) -> T { return -(3.0 / 64.0) * sqrt(385.0 / (2.0 * PI)) * pow(sin(theta), 5) * (13.0 * pow(cos(theta), 2) - 1.0); };
                case 6:
                    return [PI](T theta) -> T { return (3.0 / 64.0) * sqrt(5005.0 / PI) * pow(sin(theta), 6) * cos(theta); };
                case 7:
                    return [PI](T theta) -> T { return -(3.0 / 64.0) * sqrt(715.0 / (2.0 * PI)) * pow(sin(theta), 7); };
            }
            break;

        case 8:
            switch (order)
            {
                case -8:
                    return [PI](T theta) -> T { return (3.0 / 256.0) * sqrt(12155.0 / (2.0 * PI)) * pow(sin(theta), 8); };
                case -7:
                    return [PI](T theta) -> T { return (3.0 / 64.0) * sqrt(12155.0 / (2.0 * PI)) * pow(sin(theta), 7) * cos(theta); };
                case -6:
                    return [PI](T theta) -> T { return (1.0 / 128.0) * sqrt(7293.0 / PI) * pow(sin(theta), 6) * (15.0 * pow(cos(theta), 2) - 1.0); };
                case -5:
                    return [PI](T theta) -> T { return (3.0 / 64.0) * sqrt(17017.0 / (2.0 * PI)) * pow(sin(theta), 5) * (5.0 * pow(cos(theta), 3) - cos(theta)); };
                case -4:
                    return [PI](T theta) -> T { return (3.0 / 128.0) * sqrt(1309.0 / (2.0 * PI)) * pow(sin(theta), 4) * (65.0 * pow(cos(theta), 4) - 26.0 * pow(cos(theta), 2) + 1.0); };
                case -3:
                    return [PI](T theta) -> T { return (1.0 / 64.0) * sqrt(19635.0 / (2.0 * PI)) * pow(sin(theta), 3) * (39.0 * pow(cos(theta), 5) - 26.0 * pow(cos(theta), 3) + 3.0 * cos(theta)); };
                case -2:
                    return [PI](T theta) -> T { return (3.0 / 128.0) * sqrt(595.0 / PI) * pow(sin(theta), 2) * (143.0 * pow(cos(theta), 6) - 143.0 * pow(cos(theta), 4) + 33.0 * pow(cos(theta), 2) - 1.0); };
                case -1:
                    return [PI](T theta) -> T { return (3.0 / 64.0) * sqrt(17.0 / (2.0 * PI)) * sin(theta) * (715.0 * pow(cos(theta), 7) - 1001.0 * pow(cos(theta), 5) + 385.0 * pow(cos(theta), 3) - 35.0 * cos(theta)); };
                case 0:
                    return [PI](T theta) -> T { return (1.0 / 256.0) * sqrt(17.0 / PI) * (6435.0 * pow(cos(theta), 8) - 12012.0 * pow(cos(theta), 6) + 6930.0 * pow(cos(theta), 4) - 1260.0 * pow(cos(theta), 2) + 35.0); };
                case 1:
                    return [PI](T theta) -> T { return -(3.0 / 64.0) * sqrt(17.0 / (2.0 * PI)) * sin(theta) * (715.0 * pow(cos(theta), 7) - 1001.0 * pow(cos(theta), 5) + 385.0 * pow(cos(theta), 3) - 35.0 * cos(theta)); };
                case 2:
                    return [PI](T theta) -> T { return (3.0 / 128.0) * sqrt(595.0 / PI) * pow(sin(theta), 2) * (143.0 * pow(cos(theta), 6) - 143.0 * pow(cos(theta), 4) + 33.0 * pow(cos(theta), 2) - 1.0); };
                case 3:
                    return [PI](T theta) -> T { return -(1.0 / 64.0) * sqrt(19635.0 / (2.0 * PI)) * pow(sin(theta), 3) * (39.0 * pow(cos(theta), 5) - 26.0 * pow(cos(theta), 3) + 3.0 * cos(theta)); };
                case 4:
                    return [PI](T theta) -> T { return (3.0 / 128.0) * sqrt(1309.0 / (2.0 * PI)) * pow(sin(theta), 4) * (65.0 * pow(cos(theta), 4) - 26.0 * pow(cos(theta), 2) + 1.0); };
                case 5:
                    return [PI](T theta) -> T { return -(3.0 / 64.0) * sqrt(17017.0 / (2.0 * PI)) * pow(sin(theta), 5) * (5.0 * pow(cos(theta), 3) - cos(theta)); };
                case 6:
                    return [PI](T theta) -> T { return (1.0 / 128.0) * sqrt(7293.0 / PI) * pow(sin(theta), 6) * (15.0 * pow(cos(theta), 2) - 1.0); };
                case 7:
                    return [PI](T theta) -> T { return -(3.0 / 64.0) * sqrt(12155.0 / (2.0 * PI)) * pow(sin(theta), 7) * cos(theta); };
                case 8:
                    return [PI](T theta) -> T { return (3.0 / 256.0) * sqrt(12155.0 / (2.0 * PI)) * pow(sin(theta), 8); };
            }
            break;

        case 9:
            switch (order)
            {
                case -9:
                    return [PI](T theta) -> T { return (1.0 / 512.0) * sqrt(230945.0 / PI) * pow(sin(theta), 9); };
                case -8:
                    return [PI](T theta) -> T { return (3.0 / 256.0) * sqrt(230945.0 / (2.0 * PI)) * pow(sin(theta), 8) * cos(theta); };
                case -7:
                    return [PI](T theta) -> T { return (3.0 / 512.0) * sqrt(13585.0 / PI) * pow(sin(theta), 7) * (17.0 * pow(cos(theta), 2) - 1.0); };
                case -6:
                    return [PI](T theta) -> T { return (1.0 / 128.0) * sqrt(40755.0 / PI) * pow(sin(theta), 6) * (17.0 * pow(cos(theta), 3) - 3.0 * cos(theta)); };
                case -5:
                    return [PI](T theta) -> T { return (3.0 / 256.0) * sqrt(2717.0 / PI) * pow(sin(theta), 5) * (85.0 * pow(cos(theta), 4) - 30.0 * pow(cos(theta), 2) + 1.0); };
                case -4:
                    return [PI](T theta) -> T { return (3.0 / 128.0) * sqrt(95095.0 / (2.0 * PI)) * pow(sin(theta), 4) * (17.0 * pow(cos(theta), 5) - 10.0 * pow(cos(theta), 3)+ cos(theta)); };
                case -3:
                    return [PI](T theta) -> T { return (1.0 / 256.0) * sqrt(21945.0 / PI) * pow(sin(theta), 3) * (221.0 * pow(cos(theta), 6) - 195.0 * pow(cos(theta), 4) + 39.0 * pow(cos(theta), 2) - 1.0); };
                case -2:
                    return [PI](T theta) -> T { return (3.0 / 128.0) * sqrt(1045.0 / PI) * pow(sin(theta), 2) * (221.0 * pow(cos(theta), 7) - 273.0 * pow(cos(theta), 5) + 91.0 * pow(cos(theta), 3) - 7.0 * cos(theta)); };
                case -1:
                    return [PI](T theta) -> T { return (3.0 / 256.0) * sqrt(95.0 / (2.0 * PI)) * sin(theta) * (2431.0 * pow(cos(theta), 8) - 4004.0 * pow(cos(theta), 6) + 2002.0 * pow(cos(theta), 4) - 308.0 * pow(cos(theta), 2) + 7.0); };
                case 0:
                    return [PI](T theta) -> T { return (1.0 / 256.0) * sqrt(19.0 / PI) * (12155.0 * pow(cos(theta), 9) - 25740.0 * pow(cos(theta), 7) + 18018.0 * pow(cos(theta), 5) - 4620.0 * pow(cos(theta), 3) + 315.0 * cos(theta)); };
                case 1:
                    return [PI](T theta) -> T { return -(3.0 / 256.0) * sqrt(95.0 / (2.0 * PI)) * sin(theta) * (2431.0 * pow(cos(theta), 8) - 4004.0 * pow(cos(theta), 6) + 2002.0 * pow(cos(theta), 4) - 308.0 * pow(cos(theta), 2) + 7.0); };
                case 2:
                    return [PI](T theta) -> T { return (3.0 / 128.0) * sqrt(1045.0 / PI) * pow(sin(theta), 2) * (221.0 * pow(cos(theta), 7) - 273.0 * pow(cos(theta), 5) + 91.0 * pow(cos(theta), 3) - 7.0 * cos(theta)); };
                case 3:
                    return [PI](T theta) -> T { return -(1.0 / 256.0) * sqrt(21945.0 / PI) * pow(sin(theta), 3) * (221.0 * pow(cos(theta), 6) - 195.0 * pow(cos(theta), 4) + 39.0 * pow(cos(theta), 2) - 1.0); };
                case 4:
                    return [PI](T theta) -> T { return (3.0 / 128.0) * sqrt(95095.0 / (2.0 * PI)) * pow(sin(theta), 4) * (17.0 * pow(cos(theta), 5) - 10.0 * pow(cos(theta), 3)+ cos(theta)); };
                case 5:
                    return [PI](T theta) -> T { return -(3.0 / 256.0) * sqrt(2717.0 / PI) * pow(sin(theta), 5) * (85.0 * pow(cos(theta), 4) - 30.0 * pow(cos(theta), 2) + 1.0); };
                case 6:
                    return [PI](T theta) -> T { return (1.0 / 128.0) * sqrt(40755.0 / PI) * pow(sin(theta), 6) * (17.0 * pow(cos(theta), 3) - 3.0 * cos(theta)); };
                case 7:
                    return [PI](T theta) -> T { return -(3.0 / 512.0) * sqrt(13585.0 / PI) * pow(sin(theta), 7) * (17.0 * pow(cos(theta), 2) - 1.0); };
                case 8:
                    return [PI](T theta) -> T { return (3.0 / 256.0) * sqrt(230945.0 / (2.0 * PI)) * pow(sin(theta), 8) * cos(theta); };
                case 9:
                    return [PI](T theta) -> T { return -(1.0 / 512.0) * sqrt(230945.0 / PI) * pow(sin(theta), 9); };
            }
            break;

        case 10:
            switch (order)
            {
                case -10:
                    return [PI](T theta) -> T { return (1.0 / 1024.0) * sqrt(969969.0 / PI) * pow(sin(theta), 10); };
                case -9:
                    return [PI](T theta) -> T { return (1.0 / 512.0) * sqrt(4849845.0 / PI) * pow(sin(theta), 9) * cos(theta); };
                case -8:
                    return [PI](T theta) -> T { return (1.0 / 512.0) * sqrt(255255.0 / (2.0 * PI)) * pow(sin(theta), 8) * (19.0 * pow(cos(theta), 2) - 1.0); };
                case -7:
                    return [PI](T theta) -> T { return (3.0 / 512.0) * sqrt(85085.0 / PI) * pow(sin(theta), 7) * (19.0 * pow(cos(theta), 3) - 3.0 * cos(theta)); };
                case -6:
                    return [PI](T theta) -> T { return (3.0 / 1024.0) * sqrt(5005.0 / PI) * pow(sin(theta), 6) * (323.0 * pow(cos(theta), 4) - 102.0 * pow(cos(theta), 2) + 3.0); };
                case -5:
                    return [PI](T theta) -> T { return (3.0 / 256.0) * sqrt(1001.0 / PI) * pow(sin(theta), 5) * (323.0 * pow(cos(theta), 5) - 170.0 * pow(cos(theta), 3) + 15.0 * cos(theta)); };
                case -4:
                    return [PI](T theta) -> T { return (3.0 / 256.0) * sqrt(5005.0 / (2.0 * PI)) * pow(sin(theta), 4) * (323.0 * pow(cos(theta), 6) - 255.0 * pow(cos(theta), 4) + 45.0 * pow(cos(theta), 2) - 1.0); };
                case -3:
                    return [PI](T theta) -> T { return (3.0 / 256.0) * sqrt(5005.0 / PI) * pow(sin(theta), 3) * (323.0 * pow(cos(theta), 7) - 357.0 * pow(cos(theta), 5) + 105.0 * pow(cos(theta), 3) - 7.0 * cos(theta)); };
                case -2:
                    return [PI](T theta) -> T { return (3.0 / 512.0) * sqrt(385.0 / (2.0 * PI)) * pow(sin(theta), 2) * (4199.0 * pow(cos(theta), 8) - 6188.0 * pow(cos(theta), 6) + 2730.0 * pow(cos(theta), 4) - 364.0 * pow(cos(theta), 2) + 7.0); };
                case -1:
                    return [PI](T theta) -> T { return (1.0 / 256.0) * sqrt(1155.0 / (2.0 * PI)) * sin(theta) * (4199.0 * pow(cos(theta), 9) - 7956.0 * pow(cos(theta), 7) + 4914.0 * pow(cos(theta), 5) - 1092.0 * pow(cos(theta), 3) + 63.0 * cos(theta)); };
                case 0:
                    return [PI](T theta) -> T { return (1.0 / 512.0) * sqrt(21.0 / PI) * (46189.0 * pow(cos(theta), 10) - 109395.0 * pow(cos(theta), 8) + 90090.0 * pow(cos(theta), 6) - 30030.0 * pow(cos(theta), 4) + 3465.0 * pow(cos(theta), 2) - 63.0); };
                case 1:
                    return [PI](T theta) -> T { return -(1.0 / 256.0) * sqrt(1155.0 / (2.0 * PI)) * sin(theta) * (4199.0 * pow(cos(theta), 9) - 7956.0 * pow(cos(theta), 7) + 4914.0 * pow(cos(theta), 5) - 1092.0 * pow(cos(theta), 3) + 63.0 * cos(theta)); };
                case 2:
                    return [PI](T theta) -> T { return (3.0 / 512.0) * sqrt(385.0 / (2.0 * PI)) * pow(sin(theta), 2) * (4199.0 * pow(cos(theta), 8) - 6188.0 * pow(cos(theta), 6) + 2730.0 * pow(cos(theta), 4) - 364.0 * pow(cos(theta), 2) + 7.0); };
                case 3:
                    return [PI](T theta) -> T { return -(3.0 / 256.0) * sqrt(5005.0 / PI) * pow(sin(theta), 3) * (323.0 * pow(cos(theta), 7) - 357.0 * pow(cos(theta), 5) + 105.0 * pow(cos(theta), 3) - 7.0 * cos(theta)); };
                case 4:
                    return [PI](T theta) -> T { return (3.0 / 256.0) * sqrt(5005.0 / (2.0 * PI)) * pow(sin(theta), 4) * (323.0 * pow(cos(theta), 6) - 255.0 * pow(cos(theta), 4) + 45.0 * pow(cos(theta), 2) - 1.0); };
                case 5:
                    return [PI](T theta) -> T { return -(3.0 / 256.0) * sqrt(1001.0 / PI) * pow(sin(theta), 5) * (323.0 * pow(cos(theta), 5) - 170.0 * pow(cos(theta), 3) + 15.0 * cos(theta)); };
                case 6:
                    return [PI](T theta) -> T { return (3.0 / 1024.0) * sqrt(5005.0 / PI) * pow(sin(theta), 6) * (323.0 * pow(cos(theta), 4) - 102.0 * pow(cos(theta), 2) + 3.0); };
                case 7:
                    return [PI](T theta) -> T { return -(3.0 / 512.0) * sqrt(85085.0 / PI) * pow(sin(theta), 7) * (19.0 * pow(cos(theta), 3) - 3.0 * cos(theta)); };
                case 8:
                    return [PI](T theta) -> T { return (1.0 / 512.0) * sqrt(255255.0 / (2.0 * PI)) * pow(sin(theta), 8) * (19.0 * pow(cos(theta), 2) - 1.0); };
                case 9:
                    return [PI](T theta) -> T { return -(1.0 / 512.0) * sqrt(4849845.0 / PI) * pow(sin(theta), 9) * cos(theta); };
                case 10:
                    return [PI](T theta) -> T { return (1.0 / 1024.0) * sqrt(969969.0 / PI) * pow(sin(theta), 10); };
            }
            break;
    }

    assert(false);
    return [](auto&&...){return 0.0;};
}

//------------------------------------------------------------------------------
template<typename T>
std::function<T(T)> quetzal::math::detail::real_azimuthal_component(int order)
{
    if (order > 0)
    {
        return [order](T phi) { return cos(order * phi); };
    }

    if (order < 0)
    {
        return [order](T phi) { return sin(-order * phi); };
    }

    return [](T) { return 1.0; };
}

//------------------------------------------------------------------------------
template<typename T>
std::function<T(T)> quetzal::math::detail::imaginary_azimuthal_component(int order)
{
    if (order > 0)
    {
        return [order](T phi) { return -sin(order * phi); };
    }

    if (order < 0)
    {
        return [order](T phi) { return cos(-order * phi); };
    }

    return [](T) { return 0.0; };
}

#endif // QUETZAL_MATH_SPHERICAL_HARMONICS_HPP
