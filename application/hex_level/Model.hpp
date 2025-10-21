#if !defined(QUETZAL_HEX_LEVEL_MODEL_HPP)
#define QUETZAL_HEX_LEVEL_MODEL_HPP
//------------------------------------------------------------------------------
// Model.hpp
//------------------------------------------------------------------------------

#include "Position.hpp"
#include "Model.hpp"
#include <memory>

namespace quetzal
{

namespace hex_level
{

    //--------------------------------------------------------------------------
    class Model
    {
    public:

        Model();
        Model(const Model&) = default;
        ~Model() = default;

        Model& operator=(const Model&) = default;

        Position normal() const; // ...

        void set_normal(float x, float y, float z);

    private:

        Position m_normal; // ...

        // geometry: vertices, indices, textures ...
    };

} // namespace hex_level

} // namespace quetzal

#endif // QUETZAL_HEX_LEVEL_MODEL_HPP
