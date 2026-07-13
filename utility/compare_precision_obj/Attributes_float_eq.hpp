#if !defined(QUETZAL_GEOMETRY_ATTRIBUTES_HPP)
#define QUETZAL_GEOMETRY_ATTRIBUTES_HPP
//------------------------------------------------------------------------------
// geometry
// Attributes.hpp
//------------------------------------------------------------------------------

#include "Point.hpp"
#include "relationships.hpp"
#include "transformation_matrix.hpp"
#include "quetzal/math/floating_point.hpp"
#include "quetzal/math/math_util.hpp"
#include <functional>
#include <ostream>
#include <sstream>
#include <string>
#include <cstdint>

namespace quetzal::geometry
{

    //--------------------------------------------------------------------------
    class AttributesFlags
    {
    public:

        static constexpr uint32_t Position = 0x01;
        static constexpr uint32_t Normal = 0x02;
        static constexpr uint32_t Texcoord0 = 0x04;
        static constexpr uint32_t Texcoord1 = 0x08;
        static constexpr uint32_t Tangent = 0x10;
        static constexpr uint32_t Bitangent = 0x20;
        static constexpr uint32_t Color = 0x40;

        AttributesFlags() = default;
        constexpr AttributesFlags(uint32_t flags);
        AttributesFlags(const AttributesFlags&) = default;
        AttributesFlags(AttributesFlags&&) = default;
        ~AttributesFlags() = default;

        AttributesFlags& operator=(const AttributesFlags&) = default;
        AttributesFlags& operator=(AttributesFlags&&) = default;

        constexpr operator uint32_t() const;

    private:

        const uint32_t m_flags;
    };

    //--------------------------------------------------------------------------
    template<typename Traits, typename C = uint32_t> // ...
    class Attributes
    {
    public:

        using traits_type = Traits;
        using texcoord_traits = Traits::template resized_traits<Traits::dimension - 1>;

        using value_type = Traits::value_type;
        using point_type = Point<Traits>;
        using vector_type = point_type::vector_type;
        using texcoord_type = Point<texcoord_traits>;
        using matrix_type = transformation_matrix_type<Traits>;
        using transform_texcoord_type = std::function<texcoord_type(const texcoord_type&)>;
        using color_type = C;

        Attributes() = default;
        Attributes(const Attributes&) = default;
        Attributes(Attributes&&) = default;
        virtual ~Attributes() = default;

        Attributes& operator=(const Attributes&) = default;
        Attributes& operator=(Attributes&&) = default;

        virtual void transform(const matrix_type& matrix) = 0;
        virtual void transform(const matrix_type& matrixPosition, const matrix_type& matrixDirection) = 0;
        virtual void transform_position(const matrix_type& matrix) = 0;
        virtual void invert(transform_texcoord_type transform_texcoord) = 0;
        virtual bool validate() const = 0;
        virtual bool validate(std::string& errors) const = 0;

        // virtual instead of in addition?
        // free functions instead?

        static texcoord_type transform_texcoord_identity(const texcoord_type& texcoord);
        static texcoord_type transform_texcoord_reflect_u(const texcoord_type& texcoord);
        static texcoord_type transform_texcoord_reflect_v(const texcoord_type& texcoord);

        static bool validate_direction(const vector_type& v);
        static bool validate_direction(const vector_type& v, const std::string& element, std::string& error);
        static bool validate_texcoord(const texcoord_type& texcoord);
        static bool validate_texcoord(const texcoord_type& texcoord, std::string& error);
    };

    template<quetzal::geometry::Positional A, quetzal::geometry::Positional B>
    bool coincident(const A& a, const B& b);

    //--------------------------------------------------------------------------
    template<typename Traits>
    class Position : public Attributes<Traits>
    {
    public:

        using typename Attributes<Traits>::value_type;
        using typename Attributes<Traits>::point_type;
        using typename Attributes<Traits>::vector_type;
        using typename Attributes<Traits>::texcoord_type;
        using typename Attributes<Traits>::matrix_type;
        using typename Attributes<Traits>::transform_texcoord_type;
        using Attributes<Traits>::transform_texcoord_identity;

        Position() = default;
        Position(const point_type& position);
        Position(const Position&) = default;
        Position(Position&&) = default;
        ~Position() = default;

        Position& operator=(const Position&) = default;
        Position& operator=(Position&&) = default;

        point_type position() const;
        point_type& position();
        void set_position(const point_type& position);

        void set(const point_type& position);

        virtual void transform(const matrix_type& matrix) override;
        virtual void transform(const matrix_type& matrixPosition, const matrix_type& matrixDirection) override;
        virtual void transform_position(const matrix_type& matrix) override;
        virtual void invert(transform_texcoord_type transform_texcoord = transform_texcoord_identity) override;
        virtual bool validate() const override;
        virtual bool validate(std::string& errors) const override;

        static constexpr bool contains(AttributesFlags components);

    private:

        point_type m_position;
    };

    template<typename Traits>
    Position<Traits> lerp(const Position<Traits>& a, const Position<Traits>& b, typename Traits::value_type t);

    template<typename Traits>
    bool float_eq(const Position<Traits>& lhs, const Position<Traits>& rhs, int tolerance = math::tolerance_default);

    template<typename Traits>
    std::ostream& operator<<(std::ostream& os, const Position<Traits>& a);

    //--------------------------------------------------------------------------
    template<typename Traits>
    class PositionNormal : public Attributes<Traits>
    {
    public:

        using typename Attributes<Traits>::value_type;
        using typename Attributes<Traits>::point_type;
        using typename Attributes<Traits>::vector_type;
        using typename Attributes<Traits>::texcoord_type;
        using typename Attributes<Traits>::matrix_type;
        using typename Attributes<Traits>::transform_texcoord_type;
        using Attributes<Traits>::transform_texcoord_identity;

        PositionNormal() = default;
        PositionNormal(const point_type& position, const vector_type& normal);
        PositionNormal(const PositionNormal&) = default;
        PositionNormal(PositionNormal&&) = default;
        ~PositionNormal() = default;

        PositionNormal& operator=(const PositionNormal&) = default;
        PositionNormal& operator=(PositionNormal&&) = default;

        point_type position() const;
        point_type& position();
        void set_position(const point_type& position);

        vector_type normal() const;
        vector_type& normal();
        void set_normal(const vector_type& normal);

        void set(const point_type& position, const vector_type& normal);

        virtual void transform(const matrix_type& matrix) override;
        virtual void transform(const matrix_type& matrixPosition, const matrix_type& matrixDirection) override;
        virtual void transform_position(const matrix_type& matrix) override;
        virtual void invert(transform_texcoord_type transform_texcoord = transform_texcoord_identity) override;
        virtual bool validate() const override;
        virtual bool validate(std::string& errors) const override;

        static constexpr bool contains(AttributesFlags components);

    private:

        point_type m_position;
        vector_type m_normal;
    };

    template<typename Traits>
    PositionNormal<Traits> lerp(const PositionNormal<Traits>& a, const PositionNormal<Traits>& b, typename Traits::value_type t);

    template<typename Traits>
    bool float_eq(const PositionNormal<Traits>& lhs, const PositionNormal<Traits>& rhs, int tolerance = math::tolerance_default);

    template<typename Traits>
    std::ostream& operator<<(std::ostream& os, const PositionNormal<Traits>& a);

    //--------------------------------------------------------------------------
    template<typename Traits, typename C>
    class PositionNormalColor : public Attributes<Traits, C>
    {
    public:

        using typename Attributes<Traits, C>::value_type;
        using typename Attributes<Traits, C>::point_type;
        using typename Attributes<Traits, C>::vector_type;
        using typename Attributes<Traits, C>::texcoord_type;
        using typename Attributes<Traits, C>::matrix_type;
        using typename Attributes<Traits, C>::color_type;
        using typename Attributes<Traits>::transform_texcoord_type;
        using Attributes<Traits>::transform_texcoord_identity;

        PositionNormalColor() = default;
        PositionNormalColor(const point_type& position, const vector_type& normal, const color_type& color);
        PositionNormalColor(const PositionNormalColor&) = default;
        PositionNormalColor(PositionNormalColor&&) = default;
        ~PositionNormalColor() = default;

        PositionNormalColor& operator=(const PositionNormalColor&) = default;
        PositionNormalColor& operator=(PositionNormalColor&&) = default;

        point_type position() const;
        point_type& position();
        void set_position(const point_type& position);

        vector_type normal() const;
        vector_type& normal();
        void set_normal(const vector_type& normal);

        color_type color() const;
        color_type& color();
        void set_color(const color_type& color);

        void set(const point_type& position, const vector_type& normal, const color_type& color);

        virtual void transform(const matrix_type& matrix) override;
        virtual void transform(const matrix_type& matrixPosition, const matrix_type& matrixDirection) override;
        virtual void transform_position(const matrix_type& matrix) override;
        virtual void invert(transform_texcoord_type transform_texcoord = transform_texcoord_identity) override;
        virtual bool validate() const override;
        virtual bool validate(std::string& errors) const override;

        static constexpr bool contains(AttributesFlags components);

    private:

        point_type m_position;
        vector_type m_normal;
        color_type m_color;
    };

    template<typename Traits, typename C>
    PositionNormalColor<Traits, C> lerp(const PositionNormalColor<Traits, C>& a, const PositionNormalColor<Traits, C>& b, typename Traits::value_type t);

    template<typename Traits, typename C>
    bool float_eq(const PositionNormalColor<Traits, C>& lhs, const PositionNormalColor<Traits, C>& rhs, int tolerance = math::tolerance_default);

    template<typename Traits, typename C>
    std::ostream& operator<<(std::ostream& os, const PositionNormalColor<Traits, C>& a);

    //--------------------------------------------------------------------------
    template<typename Traits>
    class PositionNormalTexture : public Attributes<Traits>
    {
    public:

        using typename Attributes<Traits>::value_type;
        using typename Attributes<Traits>::point_type;
        using typename Attributes<Traits>::vector_type;
        using typename Attributes<Traits>::texcoord_type;
        using typename Attributes<Traits>::matrix_type;
        using typename Attributes<Traits>::transform_texcoord_type;
        using Attributes<Traits>::transform_texcoord_identity;

        PositionNormalTexture() = default;
        PositionNormalTexture(const point_type& position, const vector_type& normal, const texcoord_type& texcoord);
        PositionNormalTexture(const PositionNormalTexture&) = default;
        PositionNormalTexture(PositionNormalTexture&&) = default;
        ~PositionNormalTexture() = default;

        PositionNormalTexture& operator=(const PositionNormalTexture&) = default;
        PositionNormalTexture& operator=(PositionNormalTexture&&) = default;

        point_type position() const;
        point_type& position();
        void set_position(const point_type& position);

        vector_type normal() const;
        vector_type& normal();
        void set_normal(const vector_type& normal);

        texcoord_type texcoord() const;
        texcoord_type& texcoord();
        void set_texcoord(const texcoord_type& texcoord);

        void set(const point_type& position, const vector_type& normal, const texcoord_type& texcoord);

        virtual void transform(const matrix_type& matrix) override;
        virtual void transform(const matrix_type& matrixPosition, const matrix_type& matrixDirection) override;
        virtual void transform_position(const matrix_type& matrix) override;
        virtual void invert(transform_texcoord_type transform_texcoord = transform_texcoord_identity) override;
        virtual bool validate() const override;
        virtual bool validate(std::string& errors) const override;

        static constexpr bool contains(AttributesFlags components);

    private:

        point_type m_position;
        vector_type m_normal;
        texcoord_type m_texcoord;
    };

    template<typename Traits>
    PositionNormalTexture<Traits> lerp(const PositionNormalTexture<Traits>& a, const PositionNormalTexture<Traits>& b, typename Traits::value_type t);

    template<typename Traits>
    bool float_eq(const PositionNormalTexture<Traits>& lhs, const PositionNormalTexture<Traits>& rhs, int tolerance = math::tolerance_default);

    template<typename Traits>
    std::ostream& operator<<(std::ostream& os, const PositionNormalTexture<Traits>& a);

    //--------------------------------------------------------------------------
    template<typename Traits>
    class PositionNormalTextureTangent : public Attributes<Traits>
    {
    public:

        using typename Attributes<Traits>::value_type;
        using typename Attributes<Traits>::point_type;
        using typename Attributes<Traits>::vector_type;
        using typename Attributes<Traits>::texcoord_type;
        using typename Attributes<Traits>::matrix_type;
        using typename Attributes<Traits>::transform_texcoord_type;
        using Attributes<Traits>::transform_texcoord_identity;

        PositionNormalTextureTangent() = default;
        PositionNormalTextureTangent(const point_type& position, const vector_type& normal, const texcoord_type& texcoord, const vector_type& tangent);
        PositionNormalTextureTangent(const PositionNormalTextureTangent&) = default;
        PositionNormalTextureTangent(PositionNormalTextureTangent&&) = default;
        ~PositionNormalTextureTangent() = default;

        PositionNormalTextureTangent& operator=(const PositionNormalTextureTangent&) = default;
        PositionNormalTextureTangent& operator=(PositionNormalTextureTangent&&) = default;

        point_type position() const;
        point_type& position();
        void set_position(const point_type& position);

        vector_type normal() const;
        vector_type& normal();
        void set_normal(const vector_type& normal);

        texcoord_type texcoord() const;
        texcoord_type& texcoord();
        void set_texcoord(const texcoord_type& texcoord);

        vector_type tangent() const;
        vector_type& tangent();
        void set_tangent(const vector_type& tangent);

        void set(const point_type& position, const vector_type& normal, const texcoord_type& texcoord, const vector_type& tangent);

        virtual void transform(const matrix_type& matrix) override;
        virtual void transform(const matrix_type& matrixPosition, const matrix_type& matrixDirection) override;
        virtual void transform_position(const matrix_type& matrix) override;
        virtual void invert(transform_texcoord_type transform_texcoord = transform_texcoord_identity) override;
        virtual bool validate() const override;
        virtual bool validate(std::string& errors) const override;

        static constexpr bool contains(AttributesFlags components);

    private:

        point_type m_position;
        vector_type m_normal;
        texcoord_type m_texcoord;
        vector_type m_tangent;
    };

    template<typename Traits>
    PositionNormalTextureTangent<Traits> lerp(const PositionNormalTextureTangent<Traits>& a, const PositionNormalTextureTangent<Traits>& b, typename Traits::value_type t);

    template<typename Traits>
    bool float_eq(const PositionNormalTextureTangent<Traits>& lhs, const PositionNormalTextureTangent<Traits>& rhs, int tolerance = math::tolerance_default);

    template<typename Traits>
    std::ostream& operator<<(std::ostream& os, const PositionNormalTextureTangent<Traits>& a);

    //--------------------------------------------------------------------------
    template<typename Traits>
    class Normal : public Attributes<Traits>
    {
    public:

        using typename Attributes<Traits>::value_type;
        using typename Attributes<Traits>::point_type;
        using typename Attributes<Traits>::vector_type;
        using typename Attributes<Traits>::texcoord_type;
        using typename Attributes<Traits>::matrix_type;
        using typename Attributes<Traits>::transform_texcoord_type;
        using Attributes<Traits>::transform_texcoord_identity;

        Normal() = default;
        Normal(const vector_type& normal);
        Normal(const Normal&) = default;
        Normal(Normal&&) = default;
        ~Normal() = default;

        Normal& operator=(const Normal&) = default;
        Normal& operator=(Normal&&) = default;

        vector_type normal() const;
        vector_type& normal();
        void set_normal(const vector_type& normal);

        void set(const vector_type& normal);

        virtual void transform(const matrix_type& matrix) override;
        virtual void transform(const matrix_type& matrixPosition, const matrix_type& matrixDirection) override;
        virtual void transform_position(const matrix_type& matrix) override;
        virtual void invert(transform_texcoord_type transform_texcoord = transform_texcoord_identity) override;
        virtual bool validate() const override;
        virtual bool validate(std::string& errors) const override;

        static constexpr bool contains(AttributesFlags components);

    private:

        vector_type m_normal;
    };

    template<typename Traits>
    Normal<Traits> lerp(const Normal<Traits>& a, const Normal<Traits>& b, typename Traits::value_type t);

    template<typename Traits>
    bool float_eq(const Normal<Traits>& lhs, const Normal<Traits>& rhs, int tolerance = math::tolerance_default);

    template<typename Traits>
    std::ostream& operator<<(std::ostream& os, const Normal<Traits>& a);

    //--------------------------------------------------------------------------
    template<typename Traits>
    class NormalTangent : public Attributes<Traits>
    {
    public:

        using typename Attributes<Traits>::value_type;
        using typename Attributes<Traits>::point_type;
        using typename Attributes<Traits>::vector_type;
        using typename Attributes<Traits>::texcoord_type;
        using typename Attributes<Traits>::matrix_type;
        using typename Attributes<Traits>::transform_texcoord_type;
        using Attributes<Traits>::transform_texcoord_identity;

        NormalTangent() = default;
        NormalTangent(const vector_type& normal, const vector_type& tangent);
        NormalTangent(const NormalTangent&) = default;
        NormalTangent(NormalTangent&&) = default;
        ~NormalTangent() = default;

        NormalTangent& operator=(const NormalTangent&) = default;
        NormalTangent& operator=(NormalTangent&&) = default;

        vector_type normal() const;
        vector_type& normal();
        void set_normal(const vector_type& normal);

        vector_type tangent() const;
        vector_type& tangent();
        void set_tangent(const vector_type& tangent);

        void set(const vector_type& normal, const vector_type& tangent);

        virtual void transform(const matrix_type& matrix) override;
        virtual void transform(const matrix_type& matrixPosition, const matrix_type& matrixDirection) override;
        virtual void transform_position(const matrix_type& matrix) override;
        virtual void invert(transform_texcoord_type transform_texcoord = transform_texcoord_identity) override;
        virtual bool validate() const override;
        virtual bool validate(std::string& errors) const override;

        static constexpr bool contains(AttributesFlags components);

    private:

        vector_type m_normal;
        vector_type m_tangent;
    };

    template<typename Traits>
    NormalTangent<Traits> lerp(const NormalTangent<Traits>& a, const NormalTangent<Traits>& b, typename Traits::value_type t);

    template<typename Traits>
    bool float_eq(const NormalTangent<Traits>& lhs, const NormalTangent<Traits>& rhs, int tolerance = math::tolerance_default);

    template<typename Traits>
    std::ostream& operator<<(std::ostream& os, const NormalTangent<Traits>& a);

} // namespace quetzal::geometry

//------------------------------------------------------------------------------
constexpr quetzal::geometry::AttributesFlags::AttributesFlags(uint32_t flags) :
    m_flags(flags)
{
}

//------------------------------------------------------------------------------
constexpr quetzal::geometry::AttributesFlags::operator uint32_t() const
{
    return m_flags;
}

//------------------------------------------------------------------------------
template<typename Traits, typename C>
quetzal::geometry::Attributes<Traits, C>::texcoord_type quetzal::geometry::Attributes<Traits, C>::transform_texcoord_identity(const texcoord_type& texcoord)
{
    return texcoord;
}

//------------------------------------------------------------------------------
template<typename Traits, typename C>
quetzal::geometry::Attributes<Traits, C>::texcoord_type quetzal::geometry::Attributes<Traits, C>::transform_texcoord_reflect_u(const texcoord_type& texcoord)
{
    return {Traits::val(1) - texcoord.x(), texcoord.y()}; // Don't need the 1 ...
}

//------------------------------------------------------------------------------
template<typename Traits, typename C>
quetzal::geometry::Attributes<Traits, C>::texcoord_type quetzal::geometry::Attributes<Traits, C>::transform_texcoord_reflect_v(const texcoord_type& texcoord)
{
    return {texcoord.x(), Traits::val(1) - texcoord.y()}; // Don't need the 1 ...
}

//------------------------------------------------------------------------------
template<typename Traits, typename C>
bool quetzal::geometry::Attributes<Traits, C>::validate_direction(const vector_type& v)
{
    return v.unit();
}

//------------------------------------------------------------------------------
template<typename Traits, typename C>
bool quetzal::geometry::Attributes<Traits, C>::validate_direction(const vector_type& v, const std::string& element, std::string& error)
{
    if (validate_direction(v))
    {
        return true;
    }

    std::ostringstream oss;
    oss << "|" << element << "| != 1: " << v;
    error = oss.str();
    return false;
}

//------------------------------------------------------------------------------
template<typename Traits, typename C>
bool quetzal::geometry::Attributes<Traits, C>::validate_texcoord(const texcoord_type& texcoord)
{
    return texcoord.x() >= value_type(0) && texcoord.x() <= value_type(1) && texcoord.y() >= value_type(0) && texcoord.y() <= value_type(1); // Not a good check ...
}

//------------------------------------------------------------------------------
template<typename Traits, typename C>
bool quetzal::geometry::Attributes<Traits, C>::validate_texcoord(const texcoord_type& texcoord, std::string& error)
{
    if (validate_texcoord(texcoord))
    {
        return true;
    }

    std::ostringstream oss;
    oss << "texcoord out of range: " << texcoord;
    error = oss.str();
    return false;
}

//------------------------------------------------------------------------------
template<quetzal::geometry::Positional A, quetzal::geometry::Positional B>
bool quetzal::geometry::coincident(const A& a, const B& b)
{
    return(float_eq(a.position(), b.position()));
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Position<Traits>::Position(const point_type& position) :
    Attributes<Traits>(),
    m_position(position)
{
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::Position<Traits>::point_type quetzal::geometry::Position<Traits>::position() const
{
    return m_position;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::Position<Traits>::point_type& quetzal::geometry::Position<Traits>::position()
{
    return m_position;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::Position<Traits>::set_position(const point_type& position)
{
    m_position = position;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::Position<Traits>::set(const point_type& position)
{
    m_position = position;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::Position<Traits>::transform(const matrix_type& matrix)
{
    m_position *= matrix;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::Position<Traits>::transform(const matrix_type& matrixPosition, const matrix_type& matrixDirection)
{
    matrixDirection;
    m_position *= matrixPosition;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::Position<Traits>::transform_position(const matrix_type& matrix)
{
    m_position *= matrix;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::Position<Traits>::invert(transform_texcoord_type transform_texcoord)
{
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::Position<Traits>::validate() const
{
    return true;
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::Position<Traits>::validate(std::string& errors) const
{
    return true;
}

//------------------------------------------------------------------------------
template<typename Traits>
constexpr bool quetzal::geometry::Position<Traits>::contains(AttributesFlags components)
{
    constexpr AttributesFlags contained = AttributesFlags::Position;
    return (contained & components) == components;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Position<Traits> quetzal::geometry::lerp(const Position<Traits>& a, const Position<Traits>& b, typename Traits::value_type t)
{
    return {lerp(a.position(), b.position(), t)};
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::float_eq(const Position<Traits>& lhs, const Position<Traits>& rhs, int tolerance)
{
    return float_eq(lhs.position(), rhs.position(), tolerance);
}

//------------------------------------------------------------------------------
template<typename Traits>
std::ostream& quetzal::geometry::operator<<(std::ostream& os, const Position<Traits>& a)
{
    os << "[" << a.position() << "]";
    return os;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::PositionNormal<Traits>::PositionNormal(const point_type& position, const vector_type& normal) :
    Attributes<Traits>(),
    m_position(position),
    m_normal(normal)
{
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::PositionNormal<Traits>::point_type quetzal::geometry::PositionNormal<Traits>::position() const
{
    return m_position;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::PositionNormal<Traits>::point_type& quetzal::geometry::PositionNormal<Traits>::position()
{
    return m_position;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::PositionNormal<Traits>::set_position(const point_type& position)
{
    m_position = position;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::PositionNormal<Traits>::vector_type quetzal::geometry::PositionNormal<Traits>::normal() const
{
    return m_normal;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::PositionNormal<Traits>::vector_type& quetzal::geometry::PositionNormal<Traits>::normal()
{
    return m_normal;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::PositionNormal<Traits>::set_normal(const vector_type& normal)
{
    assert(math::float_eq(normal.norm_squared(), value_type(1)));
    m_normal = normal;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::PositionNormal<Traits>::set(const point_type& position, const vector_type& normal)
{
    m_position = position;
    m_normal = normal;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::PositionNormal<Traits>::transform(const matrix_type& matrix)
{
    transform(matrix, direction_matrix(matrix));
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::PositionNormal<Traits>::transform(const matrix_type& matrixPosition, const matrix_type& matrixDirection)
{
    m_position *= matrixPosition;
    m_normal = normalize(m_normal * matrixDirection);
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::PositionNormal<Traits>::transform_position(const matrix_type& matrix)
{
    m_position *= matrix;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::PositionNormal<Traits>::invert(transform_texcoord_type transform_texcoord)
{
    m_normal = -m_normal;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::PositionNormal<Traits>::validate() const
{
    return m_normal.unit();
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::PositionNormal<Traits>::validate(std::string& errors) const
{
    return Attributes<Traits>::validate_direction(m_normal, "normal", errors);
}

//------------------------------------------------------------------------------
template<typename Traits>
constexpr bool quetzal::geometry::PositionNormal<Traits>::contains(AttributesFlags components)
{
    constexpr AttributesFlags contained = AttributesFlags::Position | AttributesFlags::Normal;
    return (contained & components) == components;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::PositionNormal<Traits> quetzal::geometry::lerp(const PositionNormal<Traits>& a, const PositionNormal<Traits>& b, typename Traits::value_type t)
{
    return {lerp(a.position(), b.position(), t), normalize(a.normal() * slerp_unit(a.normal(), b.normal(), t))};
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::float_eq(const PositionNormal<Traits>& lhs, const PositionNormal<Traits>& rhs, int tolerance)
{
    return float_eq(lhs.position(), rhs.position(), tolerance) && float_eq(lhs.normal(), rhs.normal(), tolerance);
}

//------------------------------------------------------------------------------
template<typename Traits>
std::ostream& quetzal::geometry::operator<<(std::ostream& os, const PositionNormal<Traits>& a)
{
    os << "[" << a.position() << "]";
    os << " [" << a.normal() << "]";
    return os;
}

//------------------------------------------------------------------------------
template<typename Traits, typename C>
quetzal::geometry::PositionNormalColor<Traits, C>::PositionNormalColor(const point_type& position, const vector_type& normal, const color_type& color) :
    Attributes<Traits, C>(),
    m_position(position),
    m_normal(normal),
    m_color(color)
{
}

//------------------------------------------------------------------------------
template<typename Traits, typename C>
typename quetzal::geometry::PositionNormalColor<Traits, C>::point_type quetzal::geometry::PositionNormalColor<Traits, C>::position() const
{
    return m_position;
}

//------------------------------------------------------------------------------
template<typename Traits, typename C>
typename quetzal::geometry::PositionNormalColor<Traits, C>::point_type& quetzal::geometry::PositionNormalColor<Traits, C>::position()
{
    return m_position;
}

//------------------------------------------------------------------------------
template<typename Traits, typename C>
void quetzal::geometry::PositionNormalColor<Traits, C>::set_position(const point_type& position)
{
    m_position = position;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename C>
typename quetzal::geometry::PositionNormalColor<Traits, C>::vector_type quetzal::geometry::PositionNormalColor<Traits, C>::normal() const
{
    return m_normal;
}

//------------------------------------------------------------------------------
template<typename Traits, typename C>
typename quetzal::geometry::PositionNormalColor<Traits, C>::vector_type& quetzal::geometry::PositionNormalColor<Traits, C>::normal()
{
    return m_normal;
}

//------------------------------------------------------------------------------
template<typename Traits, typename C>
void quetzal::geometry::PositionNormalColor<Traits, C>::set_normal(const vector_type& normal)
{
    assert(math::float_eq(normal.norm_squared(), value_type(1)));
    m_normal = normal;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename C>
typename quetzal::geometry::PositionNormalColor<Traits, C>::color_type quetzal::geometry::PositionNormalColor<Traits, C>::color() const
{
    return m_color;
}

//------------------------------------------------------------------------------
template<typename Traits, typename C>
typename quetzal::geometry::PositionNormalColor<Traits, C>::color_type& quetzal::geometry::PositionNormalColor<Traits, C>::color()
{
    return m_color;
}

//------------------------------------------------------------------------------
template<typename Traits, typename C>
void quetzal::geometry::PositionNormalColor<Traits, C>::set_color(const color_type& color)
{
    m_color = color;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename C>
void quetzal::geometry::PositionNormalColor<Traits, C>::set(const point_type& position, const vector_type& normal, const color_type& color)
{
    m_position = position;
    m_normal = normal;
    m_color = color;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename C>
void quetzal::geometry::PositionNormalColor<Traits, C>::transform(const matrix_type& matrix)
{
    transform(matrix, direction_matrix(matrix));
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename C>
void quetzal::geometry::PositionNormalColor<Traits, C>::transform(const matrix_type& matrixPosition, const matrix_type& matrixDirection)
{
    m_position *= matrixPosition;
    m_normal = normalize(m_normal * matrixDirection);
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename C>
void quetzal::geometry::PositionNormalColor<Traits, C>::transform_position(const matrix_type& matrix)
{
    m_position *= matrix;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename C>
void quetzal::geometry::PositionNormalColor<Traits, C>::invert(transform_texcoord_type)
{
    m_normal = -m_normal;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename C>
bool quetzal::geometry::PositionNormalColor<Traits, C>::validate() const
{
    return m_normal.unit();
}

//------------------------------------------------------------------------------
template<typename Traits, typename C>
bool quetzal::geometry::PositionNormalColor<Traits, C>::validate(std::string& errors) const
{
    std::string error;
    bool bOK = true;

    if (!Attributes<Traits, C>::validate_direction(m_normal, "normal", error))
    {
        errors = error;
        bOK = false;
    }

    return bOK;
}

//------------------------------------------------------------------------------
template<typename Traits, typename C>
constexpr bool quetzal::geometry::PositionNormalColor<Traits, C>::contains(AttributesFlags components)
{
    constexpr AttributesFlags contained = AttributesFlags::Position | AttributesFlags::Normal | AttributesFlags::Color;
    return (contained & components) == components;
}

//------------------------------------------------------------------------------
template<typename Traits, typename C>
quetzal::geometry::PositionNormalColor<Traits, C> quetzal::geometry::lerp(const PositionNormalColor<Traits, C>& a, const PositionNormalColor<Traits, C>& b, typename Traits::value_type t)
{
    return {lerp(a.position(), b.position(), t), normalize(a.normal() * slerp_unit(a.normal(), b.normal(), t)), lerp(a.color(), b.color(), t)}; // lerp color? ...
}

//------------------------------------------------------------------------------
template<typename Traits, typename C>
bool quetzal::geometry::float_eq(const PositionNormalColor<Traits, C>& lhs, const PositionNormalColor<Traits, C>& rhs, int tolerance)
{
    return float_eq(lhs.position(), rhs.position(), tolerance) && float_eq(lhs.normal(), rhs.normal(), tolerance); // what should the color comparison be? ...
}

//------------------------------------------------------------------------------
template<typename Traits, typename C>
std::ostream& quetzal::geometry::operator<<(std::ostream& os, const PositionNormalColor<Traits, C>& a)
{
    os << "[" << a.position() << "]";
    os << " [" << a.normal() << "]";
    os << " [" << a.color() << "]";
    return os;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::PositionNormalTexture<Traits>::PositionNormalTexture(const point_type& position, const vector_type& normal, const texcoord_type& texcoord) :
    Attributes<Traits>(),
    m_position(position),
    m_normal(normal),
    m_texcoord(texcoord)
{
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::PositionNormalTexture<Traits>::point_type quetzal::geometry::PositionNormalTexture<Traits>::position() const
{
    return m_position;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::PositionNormalTexture<Traits>::point_type& quetzal::geometry::PositionNormalTexture<Traits>::position()
{
    return m_position;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::PositionNormalTexture<Traits>::set_position(const point_type& position)
{
    m_position = position;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::PositionNormalTexture<Traits>::vector_type quetzal::geometry::PositionNormalTexture<Traits>::normal() const
{
    return m_normal;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::PositionNormalTexture<Traits>::vector_type& quetzal::geometry::PositionNormalTexture<Traits>::normal()
{
    return m_normal;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::PositionNormalTexture<Traits>::set_normal(const vector_type& normal)
{
    assert(math::float_eq(normal.norm_squared(), value_type(1)));
    m_normal = normal;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::PositionNormalTexture<Traits>::texcoord_type quetzal::geometry::PositionNormalTexture<Traits>::texcoord() const
{
    return m_texcoord;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::PositionNormalTexture<Traits>::texcoord_type& quetzal::geometry::PositionNormalTexture<Traits>::texcoord()
{
    return m_texcoord;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::PositionNormalTexture<Traits>::set_texcoord(const texcoord_type& texcoord)
{
    m_texcoord = texcoord;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::PositionNormalTexture<Traits>::set(const point_type& position, const vector_type& normal, const texcoord_type& texcoord)
{
    m_position = position;
    m_normal = normal;
    m_texcoord = texcoord;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::PositionNormalTexture<Traits>::transform(const matrix_type& matrix)
{
    transform(matrix, direction_matrix(matrix));
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::PositionNormalTexture<Traits>::transform(const matrix_type& matrixPosition, const matrix_type& matrixDirection)
{
    m_position *= matrixPosition;
    m_normal = normalize(m_normal * matrixDirection);
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::PositionNormalTexture<Traits>::transform_position(const matrix_type& matrix)
{
    m_position *= matrix;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::PositionNormalTexture<Traits>::invert(transform_texcoord_type transform_texcoord)
{
    m_normal = -m_normal;
    m_texcoord = transform_texcoord(m_texcoord);
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::PositionNormalTexture<Traits>::validate() const
{
    return m_normal.unit() && Attributes<Traits>::validate_texcoord(m_texcoord);
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::PositionNormalTexture<Traits>::validate(std::string& errors) const
{
    std::string error;
    bool bOK = true;

    if (!Attributes<Traits>::validate_direction(m_normal, "normal", error))
    {
        errors = error;
        bOK = false;
    }

    if (!Attributes<Traits>::validate_texcoord(m_texcoord, error))
    {
        if (!errors.empty())
        {
            errors += "; ";
        }

        errors += error;
        bOK = false;
    }

    return bOK;
}

//------------------------------------------------------------------------------
template<typename Traits>
constexpr bool quetzal::geometry::PositionNormalTexture<Traits>::contains(AttributesFlags components)
{
    constexpr AttributesFlags contained = AttributesFlags::Position | AttributesFlags::Normal | AttributesFlags::Texcoord0;
    return (contained & components) == components;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::PositionNormalTexture<Traits> quetzal::geometry::lerp(const PositionNormalTexture<Traits>& a, const PositionNormalTexture<Traits>& b, typename Traits::value_type t)
{
    return {lerp(a.position(), b.position(), t), normalize(a.normal() * slerp_unit(a.normal(), b.normal(), t)), lerp(a.texcoord(), b.texcoord(), t)};
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::float_eq(const PositionNormalTexture<Traits>& lhs, const PositionNormalTexture<Traits>& rhs, int tolerance)
{
    return float_eq(lhs.position(), rhs.position(), tolerance) && float_eq(lhs.normal(), rhs.normal(), tolerance) && float_eq(lhs.texcoord(), rhs.texcoord(), tolerance);
}

//------------------------------------------------------------------------------
template<typename Traits>
std::ostream& quetzal::geometry::operator<<(std::ostream& os, const PositionNormalTexture<Traits>& a)
{
    os << "[" << a.position() << "]";
    os << " [" << a.normal() << "]";
    os << " [" << a.texcoord() << "]";
    return os;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::PositionNormalTextureTangent<Traits>::PositionNormalTextureTangent(const point_type& position, const vector_type& normal, const texcoord_type& texcoord, const vector_type& tangent) :
    Attributes<Traits>(),
    m_position(position),
    m_normal(normal),
    m_texcoord(texcoord),
    m_tangent(tangent)
{
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::PositionNormalTextureTangent<Traits>::point_type quetzal::geometry::PositionNormalTextureTangent<Traits>::position() const
{
    return m_position;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::PositionNormalTextureTangent<Traits>::point_type& quetzal::geometry::PositionNormalTextureTangent<Traits>::position()
{
    return m_position;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::PositionNormalTextureTangent<Traits>::set_position(const point_type& position)
{
    m_position = position;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::PositionNormalTextureTangent<Traits>::vector_type quetzal::geometry::PositionNormalTextureTangent<Traits>::normal() const
{
    return m_normal;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::PositionNormalTextureTangent<Traits>::vector_type& quetzal::geometry::PositionNormalTextureTangent<Traits>::normal()
{
    return m_normal;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::PositionNormalTextureTangent<Traits>::set_normal(const vector_type& normal)
{
    assert(math::float_eq(normal.norm_squared(), value_type(1)));
    m_normal = normal;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::PositionNormalTextureTangent<Traits>::texcoord_type quetzal::geometry::PositionNormalTextureTangent<Traits>::texcoord() const
{
    return m_texcoord;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::PositionNormalTextureTangent<Traits>::texcoord_type& quetzal::geometry::PositionNormalTextureTangent<Traits>::texcoord()
{
    return m_texcoord;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::PositionNormalTextureTangent<Traits>::set_texcoord(const texcoord_type& texcoord)
{
    m_texcoord = texcoord;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::PositionNormalTextureTangent<Traits>::vector_type quetzal::geometry::PositionNormalTextureTangent<Traits>::tangent() const
{
    return m_tangent;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::PositionNormalTextureTangent<Traits>::vector_type& quetzal::geometry::PositionNormalTextureTangent<Traits>::tangent()
{
    return m_tangent;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::PositionNormalTextureTangent<Traits>::set_tangent(const vector_type& tangent)
{
    m_tangent = tangent;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::PositionNormalTextureTangent<Traits>::set(const point_type& position, const vector_type& normal, const texcoord_type& texcoord, const vector_type& tangent)
{
    m_position = position;
    m_normal = normal;
    m_texcoord = texcoord;
    m_tangent = tangent;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::PositionNormalTextureTangent<Traits>::transform(const matrix_type& matrix)
{
    transform(matrix, direction_matrix(matrix));
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::PositionNormalTextureTangent<Traits>::transform(const matrix_type& matrixPosition, const matrix_type& matrixDirection)
{
    m_position *= matrixPosition;
    m_normal = normalize(m_normal * matrixDirection);
    m_tangent = normalize(m_tangent * matrixDirection);
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::PositionNormalTextureTangent<Traits>::transform_position(const matrix_type& matrix)
{
    m_position *= matrix;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::PositionNormalTextureTangent<Traits>::invert(transform_texcoord_type transform_texcoord)
{
    m_normal = -m_normal;
    m_texcoord = transform_texcoord(m_texcoord);
    m_tangent = -m_tangent;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::PositionNormalTextureTangent<Traits>::validate() const
{
    return m_normal.unit() && Attributes<Traits>::validate_texcoord(m_texcoord) && m_tangent.unit();
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::PositionNormalTextureTangent<Traits>::validate(std::string& errors) const
{
    std::string error;
    bool bOK = true;

    if (!Attributes<Traits>::validate_direction(m_normal, "normal", error))
    {
        errors = error;
        bOK = false;
    }

    if (!Attributes<Traits>::validate_texcoord(m_texcoord, error))
    {
        if (!errors.empty())
        {
            errors += "; ";
        }

        errors += error;
        bOK = false;
    }

    if (!Attributes<Traits>::validate_direction(m_tangent, "tangent", error))
    {
        if (!errors.empty())
        {
            errors += "; ";
        }

        errors += error;
        bOK = false;
    }

    return bOK;
}

//------------------------------------------------------------------------------
template<typename Traits>
constexpr bool quetzal::geometry::PositionNormalTextureTangent<Traits>::contains(AttributesFlags components)
{
    constexpr AttributesFlags contained = AttributesFlags::Position | AttributesFlags::Normal | AttributesFlags::Texcoord0 | AttributesFlags::Tangent;
    return (contained & components) == components;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::PositionNormalTextureTangent<Traits> quetzal::geometry::lerp(const PositionNormalTextureTangent<Traits>& a, const PositionNormalTextureTangent<Traits>& b, typename Traits::value_type t)
{
    return {lerp(a.position(), b.position(), t), normalize(a.normal() * slerp_unit(a.normal(), b.normal(), t)), lerp(a.texcoord(), b.texcoord(), t), normalize(a.tangent() * slerp_unit(a.tangent(), b.tangent(), t))};
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::float_eq(const PositionNormalTextureTangent<Traits>& lhs, const PositionNormalTextureTangent<Traits>& rhs, int tolerance)
{
    return float_eq(lhs.position(), rhs.position(), tolerance) && float_eq(lhs.normal(), rhs.normal(), tolerance) && float_eq(lhs.texcoord(), rhs.texcoord(), tolerance) && float_eq(lhs.tangent(), rhs.tangent(), tolerance);
}

//------------------------------------------------------------------------------
template<typename Traits>
std::ostream& quetzal::geometry::operator<<(std::ostream& os, const PositionNormalTextureTangent<Traits>& a)
{
    os << "[" << a.position() << "]";
    os << " [" << a.normal() << "]";
    os << " [" << a.texcoord() << "]";
    os << " [" << a.tangent() << "]";
    return os;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Normal<Traits>::Normal(const vector_type& normal) :
    Attributes<Traits>(),
    m_normal(normal)
{
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::Normal<Traits>::vector_type quetzal::geometry::Normal<Traits>::normal() const
{
    return m_normal;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::Normal<Traits>::vector_type& quetzal::geometry::Normal<Traits>::normal()
{
    return m_normal;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::Normal<Traits>::set_normal(const vector_type& normal)
{
    assert(math::float_eq(normal.norm_squared(), value_type(1)));
    m_normal = normal;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::Normal<Traits>::set(const vector_type& normal)
{
    m_normal = normal;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::Normal<Traits>::transform(const matrix_type& matrix)
{
    transform(matrix, direction_matrix(matrix));
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::Normal<Traits>::transform(const matrix_type& matrixPosition, const matrix_type& matrixDirection)
{
    matrixPosition;
    m_normal = normalize(m_normal * matrixDirection);
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::Normal<Traits>::transform_position(const matrix_type& matrix)
{
    matrix;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::Normal<Traits>::invert(transform_texcoord_type transform_texcoord)
{
    m_normal = -m_normal;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::Normal<Traits>::validate() const
{
    return m_normal.unit();
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::Normal<Traits>::validate(std::string& errors) const
{
    return Attributes<Traits>::validate_direction(m_normal, "normal", errors);
}

//------------------------------------------------------------------------------
template<typename Traits>
constexpr bool quetzal::geometry::Normal<Traits>::contains(AttributesFlags components)
{
    constexpr AttributesFlags contained = AttributesFlags::Normal;
    return (contained & components) == components;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::Normal<Traits> quetzal::geometry::lerp(const Normal<Traits>& a, const Normal<Traits>& b, typename Traits::value_type t)
{
    return {normalize(a.normal() * slerp_unit(a.normal(), b.normal(), t))};
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::float_eq(const Normal<Traits>& lhs, const Normal<Traits>& rhs, int tolerance)
{
    return float_eq(lhs.normal(), rhs.normal(), tolerance);
}

//------------------------------------------------------------------------------
template<typename Traits>
std::ostream& quetzal::geometry::operator<<(std::ostream& os, const Normal<Traits>& a)
{
    os << "[" << a.normal() << "]";
    return os;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::NormalTangent<Traits>::NormalTangent(const vector_type& normal, const vector_type& tangent) :
    Attributes<Traits>(),
    m_normal(normal),
    m_tangent(tangent)
{
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::NormalTangent<Traits>::vector_type quetzal::geometry::NormalTangent<Traits>::normal() const
{
    return m_normal;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::NormalTangent<Traits>::vector_type& quetzal::geometry::NormalTangent<Traits>::normal()
{
    return m_normal;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::NormalTangent<Traits>::set_normal(const vector_type& normal)
{
    assert(math::float_eq(normal.norm_squared(), value_type(1)));
    m_normal = normal;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::NormalTangent<Traits>::vector_type quetzal::geometry::NormalTangent<Traits>::tangent() const
{
    return m_tangent;
}

//------------------------------------------------------------------------------
template<typename Traits>
typename quetzal::geometry::NormalTangent<Traits>::vector_type& quetzal::geometry::NormalTangent<Traits>::tangent()
{
    return m_tangent;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::NormalTangent<Traits>::set_tangent(const vector_type& tangent)
{
    m_tangent = tangent;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::NormalTangent<Traits>::set(const vector_type& normal, const vector_type& tangent)
{
    m_normal = normal;
    m_tangent = tangent;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::NormalTangent<Traits>::transform(const matrix_type& matrix)
{
    transform(matrix, direction_matrix(matrix));
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::NormalTangent<Traits>::transform(const matrix_type& matrixPosition, const matrix_type& matrixDirection)
{
    matrixPosition;
    m_normal = normalize(m_normal * matrixDirection);
    m_tangent = normalize(m_tangent * matrixDirection);
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::NormalTangent<Traits>::transform_position(const matrix_type& matrix)
{
    matrix;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
void quetzal::geometry::NormalTangent<Traits>::invert(transform_texcoord_type transform_texcoord)
{
    m_normal = -m_normal;
    m_tangent = -m_tangent;
    return;
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::NormalTangent<Traits>::validate() const
{
    return m_normal.unit() && m_tangent.unit();
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::NormalTangent<Traits>::validate(std::string& errors) const
{
    std::string error;
    bool bOK = true;

    if (!Attributes<Traits>::validate_direction(m_normal, "normal", error))
    {
        errors = error;
        bOK = false;
    }

    if (!Attributes<Traits>::validate_direction(m_tangent, "tangent", error))
    {
        if (!errors.empty())
        {
            errors += "; ";
        }

        errors += error;
        bOK = false;
    }

    return bOK;
}

//------------------------------------------------------------------------------
template<typename Traits>
constexpr bool quetzal::geometry::NormalTangent<Traits>::contains(AttributesFlags components)
{
    constexpr AttributesFlags contained = AttributesFlags::Normal | AttributesFlags::Tangent;
    return (contained & components) == components;
}

//------------------------------------------------------------------------------
template<typename Traits>
quetzal::geometry::NormalTangent<Traits> quetzal::geometry::lerp(const NormalTangent<Traits>& a, const NormalTangent<Traits>& b, typename Traits::value_type t)
{
    return {normalize(a.normal() * slerp_unit(a.normal(), b.normal(), t)), normalize(a.tangent() * slerp_unit(a.tangent(), b.tangent(), t))};
}

//------------------------------------------------------------------------------
template<typename Traits>
bool quetzal::geometry::float_eq(const NormalTangent<Traits>& lhs, const NormalTangent<Traits>& rhs, int tolerance)
{
    return float_eq(lhs.normal(), rhs.normal(), tolerance) && float_eq(lhs.tangent(), rhs.tangent(), tolerance);
}

//------------------------------------------------------------------------------
template<typename Traits>
std::ostream& quetzal::geometry::operator<<(std::ostream& os, const NormalTangent<Traits>& a)
{
    os << "[" << a.normal() << "]";
    os << " [" << a.tangent() << "]";
    return os;
}

#endif // QUETZAL_GEOMETRY_ATTRIBUTES_HPP
