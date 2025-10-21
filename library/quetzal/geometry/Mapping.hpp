#if !defined(QUETZAL_GEOMETRY_MAPPING_HPP)
#define QUETZAL_GEOMETRY_MAPPING_HPP
//------------------------------------------------------------------------------
// geometry
// Mapping.hpp
//------------------------------------------------------------------------------

#include "transformation_matrix.hpp"
#include <span>

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
    assert(from.size() >= 2);
    assert(from.size() == to.size());

    std::vector<To> projected;
    std::ranges::transform(from, back_inserter(projected), [this](const From& from) { return m_projection(from); });

    switch (from.size())
    {
        case 2:
            m_matrix = linear_mapping<To>(projected, to);
            break;

        case 3:
            m_matrix = affine_mapping<To>(projected, to);
            break;

        default:
            m_matrix = projective_mapping<To>(projected, to);
            break;
    }
        
    return;
}

//------------------------------------------------------------------------------
template<typename From, typename To, typename Proj1>
To quetzal::geometry::Mapping<From, To, Proj1>::operator()(const From& point) const
{
    // projective divide? ...

    return m_projection(point) * m_matrix;
}

#endif // QUETZAL_GEOMETRY_MAPPING_HPP
