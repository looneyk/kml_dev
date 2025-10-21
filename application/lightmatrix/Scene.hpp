#if !defined(QUETZAL_LIGHTMATRIX_SCENE_HPP)
#define QUETZAL_LIGHTMATRIX_SCENE_HPP
//------------------------------------------------------------------------------
// Light Matrix Simulation
// Scene.hpp
//------------------------------------------------------------------------------

#include "Frame.hpp"
//#include "quetzal/direct3d11/Scene.hpp"
#include "quetzal/direct3d11/Camera.hpp"
#include "quetzal/direct3d11/Model.hpp"

namespace quetzal::lightmatrix
{

    class Application;

    //--------------------------------------------------------------------------
//    class Scene : public quetzal::direct3d11::Scene
    class Scene
    {
    public:

        Scene(Application& application, Frame& frame);
        Scene(const Scene&) = delete;
        ~Scene() = default;

        Scene& operator=(const Scene&) = delete;

        void load();
        void update(float dt);
        void draw(direct3d11::Camera& camera);

        size_t vertex_count() const;
        size_t triangle_count() const;

    private:

        void draw_frame(direct3d11::Camera& camera, const Frame& Frame);
        void draw_pixel(direct3d11::Camera& camera, const Pixel& pixel);

        Application& m_application;

        Frame& m_frame;

        // pixel spacing here too ...
        float m_rPixel;

        std::shared_ptr<direct3d11::Model> m_pModelPixel;

        // Statistics
        size_t m_nVertices;
        size_t m_nTriangles;
    };

} // namespace quetzal::lightmatrix

#endif // QUETZAL_LIGHTMATRIX_SCENE_HPP
