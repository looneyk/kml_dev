#if !defined(QUETZAL_HEX_LEVEL_SIZE_HPP)
#define QUETZAL_HEX_LEVEL_SIZE_HPP
//------------------------------------------------------------------------------
// Size.hpp
//------------------------------------------------------------------------------

namespace quetzal
{

namespace hex_level
{

    //--------------------------------------------------------------------------
    class Size
    {
    public:

        Size();
        Size(float dx, float dy, float dz);
        Size(const Size&) = default;
        ~Size() = default;

        Size& operator=(const Size&) = default;

        float dx();
        float dy();
        float dz();

        void set(float dx, float dy, float dz);

    private:

        float m_dx;
        float m_dy;
        float m_dz;
    };

} // namespace hex_level

} // namespace quetzal

#endif // QUETZAL_ROUGUELIKE_SIZE
