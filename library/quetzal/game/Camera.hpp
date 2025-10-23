#if !defined(QUETZAL_GAME_CAMERA_HPP)
#define QUETZAL_GAME_CAMERA_HPP
//------------------------------------------------------------------------------
// game
// Camera.hpp
//------------------------------------------------------------------------------

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#pragma warning(push)
#pragma warning(disable: 4201)
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#pragma warning(pop)
#include "types.hpp"

namespace quetzal::game
{

    //--------------------------------------------------------------------------
    class Camera
    {
    public:

        enum class ViewMode
        {
            Free,
            Fixed,
        };

        explicit Camera(float fov0 = FOV0);
        explicit Camera(ViewMode mode, float fov0 = FOV0);
        Camera(const Camera&) = delete;
        Camera(Camera&&) = delete;
        ~Camera() = default;

        Camera& operator=(const Camera&) = delete;
        Camera& operator=(Camera&&) = delete;

        ViewMode view_mode() const;
        void set_view_mode(ViewMode mode);
        void set_up(vector_type up); // Reference up direction for fixed mode

        point_type position() const;
        vector_type up() const;
        vector_type forward() const;
        vector_type right() const;

        matrix_type view() const;
        matrix_type projection() const;
        matrix_type view_projection() const;

        // View matrix

        // Call update_view after the desired set of these have been applied
        void set_position(point_type position);
        void set_forward(vector_type direction);
        void pan(float radians); // Relative rotation around effective up vector
        void tilt(float radians); // Relative rotation around right vector
        void roll(float radians); // Relative rotation around the forward vector

        void update_view();

        // Projection matrix

        // Field of view in horizontal direction
        float fov() const;
        void set_fov(float fov);

        float zoom() const;
        void set_zoom(float zoom);

        // flost size type for each ...
        void set_perspective(float aspect, float near, float far);
        void set_orthographic(float w, float h, float near, float far);

    protected:

        static constexpr float FOV0 = PI / 3.0f;

        ViewMode m_mode;
        vector_type m_up0; // reference up vector for fixed view mode

        point_type m_position;
        vector_type m_up;
        vector_type m_forward;
        vector_type m_right;

        float m_fov0;
        float m_fov;

        matrix_type m_view;
        matrix_type m_projection;
    };

} // namespace quetzal::game

#endif // QUETZAL_GAME_CAMERA_HPP
