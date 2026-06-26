#if !defined(QUETZAL_GAME_TYPES_HPP)
#define QUETZAL_GAME_TYPES_HPP
//------------------------------------------------------------------------------
// game
// types.hpp
//------------------------------------------------------------------------------

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace quetzal::game
{

    using point_type = glm::vec4;
    using vector_type = glm::vec3;
    using matrix_type = glm::mat4;

    static constexpr float PI = glm::pi<float>();

} // namespace quetzal::game

#endif // QUETZAL_GAME_TYPES_HPP
