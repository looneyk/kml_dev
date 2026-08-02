#if !defined(QUETZAL_GEOMETRY_MAPPING_HPP)
#define QUETZAL_GEOMETRY_MAPPING_HPP
//------------------------------------------------------------------------------
// geometry
// Mapping.hpp
//------------------------------------------------------------------------------

#include "Point.hpp"
#include "transformation_matrix.hpp"
#include "quetzal/math/floating_point.hpp"
#include <cassert>
#include <span>
#include <vector>

namespace quetzal::geometry
{

    // Mapping type (linear, affine, projective) is determined by the number of corresponding points passed in

    //--------------------------------------------------------------------------
    template<typename From, typename To, typename Proj1 = std::identity>
    class Mapping
    {
    public:

        Mapping();
        Mapping(std::span<const From> from, std::span<const To> to, Proj1 projection = {});
        Mapping(const Mapping&) = default;
        Mapping(Mapping&&) = default;
        ~Mapping() = default;

        Mapping& operator=(const Mapping&) = default;
        Mapping& operator=(Mapping&&) = default;

        void init(std::span<const From> from, std::span<const To> to);

        To operator()(const From& point) const;

    private:

        using matrix_type = transformation_matrix_type<typename To::traits_type>;

        matrix_type m_matrix;
        Proj1 m_projection;
    };

} // namespace quetzal::geometry

//------------------------------------------------------------------------------
template<typename From, typename To, typename Proj1>
quetzal::geometry::Mapping<From, To, Proj1>::Mapping() :
    m_matrix{matrix_type::Identity()},
    m_projection{}
{
}

//------------------------------------------------------------------------------
template<typename From, typename To, typename Proj1>
quetzal::geometry::Mapping<From, To, Proj1>::Mapping(std::span<const From> from, std::span<const To> to, Proj1 projection) :
    m_matrix{},
    m_projection{projection}
{
    init(from, to);
}

//------------------------------------------------------------------------------
template<typename From, typename To, typename Proj1>
void quetzal::geometry::Mapping<From, To, Proj1>::init(std::span<const From> from, std::span<const To> to)
{
    assert(from.size() >= To::dimension);
    assert(from.size() == to.size());

    std::vector<To> projected;
    std::ranges::transform(from, back_inserter(projected), [this](const From& from) { return m_projection(from); });
    std::span<const To> mapped_from(projected);

    // How many correspondences select which kind of map is dimension relative, not fixed at the two
    // dimensional counts: linear takes dimension of them, affine one more, projective one more again
    if (from.size() == To::dimension)
    {
        m_matrix = linear_mapping<To>(mapped_from.template first<To::dimension>(), to.template first<To::dimension>());
    }
    else if (from.size() == To::dimension + 1)
    {
        m_matrix = affine_mapping<To>(mapped_from.template first<To::dimension + 1>(), to.template first<To::dimension + 1>());
    }
    else
    {
        // Dimension + 2 determine a homography exactly, more are fitted by least squares
        if constexpr (To::dimension == 2)
        {
            m_matrix = projective_mapping<To>(mapped_from, to);
        }
        else
        {
            assert(false); // The direct linear transformation is written for two dimensions only
        }
    }

    return;
}

//------------------------------------------------------------------------------
template<typename From, typename To, typename Proj1>
To quetzal::geometry::Mapping<From, To, Proj1>::operator()(const From& point) const
{
    To to = m_projection(point) * m_matrix;
    return projective_divide(to);
}

#endif // QUETZAL_GEOMETRY_MAPPING_HPP
