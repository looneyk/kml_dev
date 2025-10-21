//------------------------------------------------------------------------------
// Light Matrix Simulation
// Scene.cpp
//------------------------------------------------------------------------------

#include "Scene.hpp"
#include "Application.hpp"
#include "quetzal/brep/Mesh.hpp"
#include "quetzal/brep/MeshTraits.hpp"
#include "quetzal/common/Exception.hpp"
#include "quetzal/direct3d11/Effect.hpp"
#include "quetzal/direct3d11/EffectManager.hpp"
#include "quetzal/direct3d11/Vertex.hpp"
#include "quetzal/direct3d11/mesh_util.hpp"
#include "quetzal/math/HomogeneousTraits.hpp"
#include "quetzal/model/primitives.hpp"

using namespace std;
using namespace DirectX;

namespace
{

    using mesh_traits = quetzal::brep::MeshTraits<quetzal::math::HomogeneousTraits<float, 3>>;
    using mesh_type = quetzal::brep::Mesh<mesh_traits>;

} // namespace
//------------------------------------------------------------------------------
quetzal::lightmatrix::Scene::Scene(Application& application, Frame& frame) :
    m_application(application),
    m_frame(frame),
    m_rPixel(0.5f),
    m_pModelPixel(),
    m_nVertices(0),
    m_nTriangles(0)
{
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::Scene::load()
{
    direct3d11::EffectManager::EffectInfo effectInfo = {};
    effectInfo.m_name = "pixel";
//    effectInfo.m_bVertexColor = true;
    effectInfo.m_bVertexColor = true;
    effectInfo.m_colorDiffuse = XMFLOAT3(1.0f, 0.0f, 1.0f);
    effectInfo.m_colorEmissive = XMFLOAT3(0.5f, 0.5f, 0.5f);
    effectInfo.m_alpha = 1.0f;

    auto pEffect = m_application.effect_manager().open(effectInfo);
    assert(pEffect != nullptr);

    mesh_type mesh;
    model::create_icosahedron(mesh, "icosahedron", m_rPixel);

//    auto [vertices, indices] = model::extract_input_layout<geometry::PositionNormalTexture<float>, int16_t>(mesh);
// ...
//    std::vector<direct3d11::VertexPositionNormalTexture> vertices;
//    std::vector<uint32_t> indices;

    //load vertices, indices from model ...
/*
    auto [vertices, indices] = direct3d11::mesh_geometry<direct3d11::VertexPositionNormalTexture, uint32_t, mesh_type>(mesh,
        [](const typename mesh_type::vertex_attributes_type& v) -> direct3d11::VertexPositionNormalTexture
        {
            return {
                direct3d11::Vector3{v.position().x(), v.position().y(), v.position().z()},
                direct3d11::Vector3{v.normal().x(), v.normal().y(), v.normal().z()},
                direct3d11::Vector2{v.texcoord().x(), v.texcoord().y()}
            };
        }
    );
*/
    auto [vertices, indices] = direct3d11::mesh_geometry<direct3d11::VertexPositionColor, uint32_t, mesh_type>(mesh,
        [](const typename mesh_type::vertex_attributes_type& v) -> direct3d11::VertexPositionColor
        {
            return {
                direct3d11::Vector3{v.position().x(), v.position().y(), v.position().z()},
                DirectX::XMFLOAT4{1.0, 1.0, 1.0, 1.0},
            };
        }
    );

    // instead, just pass vertices in as is and change emissive color in effect ...
    // either all vertices need color changes or each pixel needs separate tracked effect ...
//    vector<direct3d11::VertexPositionColor> vpcs(12);
//    transform(vertices.begin(), vertices.end(), vpcs.begin(), [](const direct3d11::geometry_vertex_type& v) -> direct3d11::VertexPositionColor { return {v.m_position, {0.0f, 0.0f, 0.0f, 1.0f}}; });
//
//    m_pModelPixel = direct3d11::Model::load(m_renderer, vpcs, indices, pEffectPixel);
//    m_pModelPixel = direct3d11::Model::load(m_renderer, vertices, indices, pEffect);
    m_pModelPixel = std::make_shared<direct3d11::Model>();
    assert(m_pModelPixel != nullptr);
    m_pModelPixel->insert(m_application.renderer(), vertices, indices, pEffect);
    return;
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::Scene::update(float dt)
{
    dt;
    return;
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::Scene::draw(direct3d11::Camera& camera)
{
    m_nVertices = 0;
    m_nTriangles = 0;

    draw_frame(camera, m_frame);
    return;
}

//------------------------------------------------------------------------------
size_t quetzal::lightmatrix::Scene::vertex_count() const
{
    return m_nVertices;
}

//------------------------------------------------------------------------------
size_t quetzal::lightmatrix::Scene::triangle_count() const
{
    return m_nTriangles;
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::Scene::draw_frame(direct3d11::Camera& camera, const Frame& frame)
{
    int w = frame.width();
    int h = frame.height();

    for (int j = 0; j < h; ++j)
    {
        for (int i = 0; i < w; ++i)
        {
            Pixel pixel = frame.pixel(i, j);

            // set effect color from pixel.color() ...

            draw_pixel(camera, pixel);
        }
    }

    return;
}

//------------------------------------------------------------------------------
void quetzal::lightmatrix::Scene::draw_pixel(direct3d11::Camera& camera, const Pixel& pixel)
{
    direct3d11::EffectManager::EffectInfo effectInfo = {};
    effectInfo.m_name = "pixel";
//    effectInfo.m_bVertexColor = true;
    effectInfo.m_bVertexColor = true;
    effectInfo.m_colorDiffuse = XMFLOAT3(1.0f, 0.0f, 1.0f);
    effectInfo.m_colorEmissive = XMFLOAT3(0.5f, 0.5f, 0.5f);
    effectInfo.m_alpha = 1.0f;

    auto pEffect = m_application.effect_manager().open(effectInfo);
    assert(pEffect != nullptr);

    // set effect here ...
    m_pModelPixel->m_meshes.front()->m_parts.front()->set_effect(m_application.renderer(), pEffect);
    m_pModelPixel->set_modified();

    DirectX::XMMATRIX mWorld = XMMatrixTranslation(pixel.x(), pixel.y(), 0.0f);
    m_pModelPixel->draw(m_application.renderer(), m_application.common_states(), mWorld, camera.view(), camera.projection());

    m_nVertices += m_pModelPixel->vertex_count();
    m_nTriangles += m_pModelPixel->triangle_count();
    return;
}
