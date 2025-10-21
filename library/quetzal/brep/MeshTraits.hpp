#if !defined(QUETZAL_BREP_MESHTRAITS_HPP)
#define QUETZAL_BREP_MESHTRAITS_HPP
//------------------------------------------------------------------------------
// brep
// MeshTraits.hpp
//------------------------------------------------------------------------------

//#include "GeometryTraits.hpp"
#include "quetzal/geometry/Attributes.hpp"

namespace quetzal::brep
{

    // NormalTangent used for Submesh default attributes represent orientation up and right

    //--------------------------------------------------------------------------
    template<typename Traits, typename V = geometry::PositionNormalTexture<Traits>, typename F = geometry::Normal<Traits>, typename S = geometry::Normal<Traits>, typename O = geometry::NormalTangent<Traits>>
    class MeshTraits
    {
    public:

        using vector_traits = Traits;
//        using geometry_traits = GeometryTraits<Traits>;
// or        using geometry_traits = Traits; // ...
        
        using vertex_attributes_type = V;
        using face_attributes_type = F;
        using surface_attributes_type = S;
        using submesh_attributes_type = O;

        using size_type = size_t;
        using value_type = Traits::value_type;
// geometry_traits instead of V ...
// or Traits is a GeometryTraits implementation ...
// but then default template parameters are a bit more complex
// at what level should GeometryTraits be, what types should it provide? ...
        using point_type = typename V::point_type;
        using vector_type = typename V::vector_type;
        using matrix_type = typename V::matrix_type;

        static constexpr size_t dimension = Traits::dimension;

        template<typename U>
        static constexpr value_type val(const U& u)
        {
            return static_cast<value_type>(u);
        }
    };

} // namespace quetzal::brep

#endif // QUETZAL_BREP_MESHTRAITS_HPP
