//------------------------------------------------------------------------------
// game
// Camera.cpp
//------------------------------------------------------------------------------

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#define GLM_FORCE_SWIZZLE
#define GLM_ENABLE_EXPERIMENTAL

#include "Camera.hpp"
#include <glm/gtx/transform.hpp>
#include <cmath>

#include <iostream>

using namespace std;
using namespace quetzal;

//------------------------------------------------------------------------------
game::Camera::Camera(float fov0) :
    m_mode(ViewMode::Free),
    m_up0(0.0f, 0.0f, 1.0f),
    m_position(0.0f, 0.0f, 0.0f, 1.0f),
    m_up(0.0f, 0.0f, 1.0f),
    m_forward(0.0f, 1.0f, 0.0f),
    m_right(1.0f, 0.0f, 0.0f),
    m_fov0(fov0),
    m_fov(fov0),
    m_view(),
    m_projection()
{
}

//------------------------------------------------------------------------------
game::Camera::Camera(ViewMode mode, float fov0) :
    m_mode(mode),
    m_up0(0.0f, 0.0f, 1.0f),
    m_position(0.0f, 0.0f, 0.0f, 1.0f),
    m_up(0.0f, 0.0f, 1.0f),
    m_forward(0.0f, 1.0f, 0.0f),
    m_right(1.0f, 0.0f, 0.0f),
    m_fov0(fov0),
    m_fov(fov0),
    m_view(),
    m_projection()
{
}

//------------------------------------------------------------------------------
game::Camera::ViewMode game::Camera::view_mode() const
{
    return m_mode;
}

//------------------------------------------------------------------------------
void game::Camera::set_view_mode(ViewMode mode)
{
    m_mode = mode;
    if (m_mode == ViewMode::Fixed)
    {
        set_forward(m_forward);
    }

    return;
}

//------------------------------------------------------------------------------
void game::Camera::set_up(vector_type up)
{
    m_up0 = up;
    if (m_mode == ViewMode::Fixed)
    {
        set_forward(m_forward);
    }

    return;
}

//------------------------------------------------------------------------------
game::point_type game::Camera::position() const
{
    return m_position;
}

//------------------------------------------------------------------------------
game::vector_type game::Camera::up() const
{
    return m_up;
}

//------------------------------------------------------------------------------
game::vector_type game::Camera::forward() const
{
    return m_forward;
}

//------------------------------------------------------------------------------
game::vector_type game::Camera::right() const
{
    return m_right;
}

//------------------------------------------------------------------------------
game::matrix_type game::Camera::view() const
{
    return m_view;
}

//------------------------------------------------------------------------------
game::matrix_type game::Camera::projection() const
{
    return m_projection;
}

//------------------------------------------------------------------------------
game::matrix_type game::Camera::view_projection() const
{
    return m_projection * m_view;
}

//------------------------------------------------------------------------------
void game::Camera::set_position(point_type position)
{
    m_position = position;
    return;
}

//------------------------------------------------------------------------------
void game::Camera::set_forward(vector_type direction)
{
    auto up = m_mode == ViewMode::Fixed ? m_up0 : m_up;
    m_forward = glm::normalize(direction);
    m_right = glm::normalize(glm::cross(m_forward, up));
    m_up = glm::normalize(glm::cross(m_right, m_forward));
    return;
}

//------------------------------------------------------------------------------
void game::Camera::pan(float radians)
{
    auto up = m_mode == ViewMode::Fixed ? m_up0 : m_up;
    matrix_type matrix = glm::rotate(radians, up);
    m_forward = glm::normalize(matrix * glm::vec4(m_forward, 0.0f));
    m_right = glm::normalize(matrix * glm::vec4(m_right, 0.0f));
    m_up = glm::normalize(glm::cross(m_right, m_forward));
    return;
}

//------------------------------------------------------------------------------
void game::Camera::tilt(float radians)
{
    matrix_type matrix = glm::rotate(radians, m_right);
    m_forward = glm::normalize(matrix * glm::vec4(m_forward, 0.0f));
    if (m_mode == ViewMode::Fixed)
    {
        m_right = glm::normalize(glm::cross(m_forward, m_up0));
        m_up = glm::normalize(glm::cross(m_right, m_forward));
    }
    else
    {
        m_up = glm::normalize(matrix * glm::vec4(m_up, 0.0f));
    }
    return;
}

//------------------------------------------------------------------------------
void game::Camera::roll(float radians)
{
    if (m_mode == ViewMode::Free)
    {
        matrix_type matrix = glm::rotate(matrix, radians, m_forward);
        m_right = glm::normalize(matrix * glm::vec4(m_right, 0.0f));
        m_up = glm::normalize(matrix * glm::vec4(m_up, 0.0f));
    }

    return;
}

//------------------------------------------------------------------------------
void game::Camera::update_view()
{
    m_view = glm::lookAt(m_position.xyz(), m_position.xyz() + m_forward, m_up);
    return;
}

//------------------------------------------------------------------------------
float game::Camera::fov() const
{
    return m_fov;
}

//------------------------------------------------------------------------------
void game::Camera::set_fov(float fov)
{
    m_fov = fov;
    return;
}

//------------------------------------------------------------------------------
float game::Camera::zoom() const
{
    return tan(m_fov0 / 2.0f) / tan(m_fov / 2.0f);
}

//------------------------------------------------------------------------------
void game::Camera::set_zoom(float zoom)
{
    m_fov = 2.0f * atan(tan(FOV0 / 2.0f) / zoom);
    return;
}

//------------------------------------------------------------------------------
void game::Camera::set_perspective(float aspect, float near, float far)
{
    m_projection = glm::perspective(m_fov / aspect, aspect, near, far);
    m_projection[1][1] *= -1; // wtf is this for? ...
    return;
}

//------------------------------------------------------------------------------
void game::Camera::set_orthographic(float w, float h, float near, float far)
{
    auto w2 = w / 2.0f;
    auto h2 = h / 2.0f;
    m_projection = glm::ortho(-w2, w2, -h2, h2, near, far);
    return;
}
