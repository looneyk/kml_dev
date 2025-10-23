#if !defined(QUETZAL_GAME_SCENE_HPP)
#define QUETZAL_GAME_SCENE_HPP

#include <functional>
#include <unordered_map>

namespace quetzal::game
{

    using hash_type = ...;
    using matrix_type = glm::mat4;

    class Surface
    {
        shader
        vertex attributes
        pipeline
    };

    using surface_cache_type = std:unordered_map<hash_type, Surface>;
    using surface_hasher_type = std::function<hash_type(const Surface&>;

    class Primitives
    {
        vertex buffer
        index buffer
    };

    using primitives_cache_type = std::unordered_map<hash_type, Primitives>;
    using primitives_hasher_type = std::function<hash_type(const Primitives&>;

    class Instance
    {
    public:

        hash_type m_surface;
        hash_type m_primitives;
        matrix_type m_matrix; // position and orientation
    };

    using instances_type = std::vector<Instance>;

    class Scene
    {
    public:

        Scene();

        using instances_type = std::vector<matrix_type>;
        using primitives_instances_type = std::unordered_map<hash_type, instances_type>;
        using surface_primitives_instances_type = std::unordered_map<hash_type, primitives_instances_type>;

    private:

        surface_primitives_instances_type m_instances;
    };


    node
        material
        name ?
        (texture, shader, pipeline, ...)
        optional<script>
        geometry
        name ?
        vertex attributes
        vector<primitives> // vertex start, count; index start, count
        optional<transform>
        optional<script>
        optional<instancing> // can include list of transform and material changes (parameterized), ned to understabd this better ...


} // namespace quetzal::game

#endif // QUETZAL_GAME_SCENE_HPP
