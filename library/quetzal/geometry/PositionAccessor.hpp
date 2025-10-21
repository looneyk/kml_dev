#if !defined(QUETZAL_GEOMETRY_POSITIONACCESSOR_HPP)
#define QUETZAL_GEOMETRY_POSITIONACCESSOR_HPP
//------------------------------------------------------------------------------
// geometry
// PositionAccessor.hpp
//------------------------------------------------------------------------------

namespace quetzal::geometry
{

    template<typename T>
    struct PositionAccessor
    {
        using point_type = T::value_type;
        using vector_type = point_type::vector_type;
        using value_type = point_type::value_type;
        static constexpr size_t dimension = point_type::dimension;

        static size_t position_count(const T& t)
        {
            return t.size();
        }

        static point_type position(const T& t, size_t n)
        {
            if (n >= t.size())
            {
                throw std::out_of_range("PositionAccessor::range_check");
            }

            return t[n];
        }
    
        static bool constant_time_acccess()
        {
            return true;
        }
    };

} // namespace quetzal::geometry

#endif // QUETZAL_GEOMETRY_POSITIONACCESSOR_HPP
