#if !defined(QUETZAL_MODEL_MESH_DIFF_HPP)
#define QUETZAL_MODEL_MESH_DIFF_HPP
//------------------------------------------------------------------------------
// model
// mesh_diff.hpp
//------------------------------------------------------------------------------

#include "quetzal/geometry/Attributes.hpp"
#include "quetzal/geometry/AxisAlignedBoundingBox.hpp"
#include "quetzal/geometry/Point.hpp"
#include "quetzal/geometry/PositionAccessor.hpp"
#include "quetzal/geometry/spatial_properties.hpp"
#include "quetzal/common/id.hpp"
#include "quetzal/math/floating_point.hpp"
#include "quetzal/wavefront_obj/Reader.hpp"
#include "quetzal/wavefront_obj/symbols.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include <cassert>
#include <cmath>

namespace quetzal::model
{

    // Comparison tolerance in ulps
    constexpr int mesh_diff_tolerance_default = 1000;

    //--------------------------------------------------------------------------
    class Options
    {
    public:

        int tolerance = mesh_diff_tolerance_default;
        bool compare_normals = true;
        bool compare_texcoords = true;
        bool compare_surfaces = true; // Require corresponding faces to share submesh and surface names
        size_t detail_limit = 16;     // Number of Difference records retained, 0 for no limit
    };

    class Comparison;

    //--------------------------------------------------------------------------
    template<typename M>
    concept BrepMesh = requires
    {
        typename M::vector_traits;
        typename M::vertex_attributes_type;
        typename M::halfedge_type;
    };

    //--------------------------------------------------------------------------
    // Compare two meshes, given as obj files, as brep::Mesh objects, or one of each.
    // The file forms need the traits to read with, which the mesh forms supply

    template<typename Traits, typename V = geometry::PositionNormalTexture<Traits>>
    Comparison mesh_diff(const std::filesystem::path& lhs, const std::filesystem::path& rhs, const Options& options = {});

    template<BrepMesh M>
    Comparison mesh_diff(const M& lhs, const M& rhs, const Options& options = {});

    template<BrepMesh M>
    Comparison mesh_diff(const std::filesystem::path& lhs, const M& rhs, const Options& options = {});

    template<BrepMesh M>
    Comparison mesh_diff(const M& lhs, const std::filesystem::path& rhs, const Options& options = {});

    //--------------------------------------------------------------------------
    // How a face in one model corresponds to its counterpart in the other
    enum class FaceMatch
    {
        identical,  // Every compared attribute is exactly equal
        equivalent, // Every compared attribute is equal within tolerance
        reversed,   // Equivalent only when one of the loops is traversed in the opposite direction
        different,  // A counterpart was found by position but its attributes differ
        unmatched   // No counterpart was found
    };

    const std::string& to_string(FaceMatch match);

    //--------------------------------------------------------------------------
    // Counts of differing vertices within a single pair of faces
    class FaceDifferences
    {
    public:

        size_t positions = 0;
        size_t normals = 0;
        size_t texcoords = 0;

        size_t total() const;
        bool empty() const;
    };

    //--------------------------------------------------------------------------
    // A single face level difference, retained for reporting
    class Difference
    {
    public:

        id_type face_id = 0; // In lhs, or in rhs when match is unmatched and face_id_other is unset
        std::optional<id_type> face_id_other;
        FaceMatch match = FaceMatch::unmatched;
        FaceDifferences differences;
        std::string surface;       // Qualified surface name in lhs
        std::string surface_other; // Qualified surface name in rhs, empty when there is no counterpart
    };

    //--------------------------------------------------------------------------
    class Comparison
    {
    public:

        size_t n_faces = 0;       // lhs
        size_t n_faces_other = 0; // rhs
        size_t n_identical = 0;
        size_t n_equivalent = 0; // Includes identical
        size_t n_reversed = 0;
        size_t n_different = 0;
        size_t n_unmatched = 0;       // lhs faces with no counterpart
        size_t n_unmatched_other = 0; // rhs faces with no counterpart
        size_t n_surface_differences = 0;

        // Faces matched as the same polygon triangulated differently, counted per side. Only those of a planar polygon are equivalent
        size_t n_triangulation = 0;
        size_t n_triangulation_nonplanar = 0;

        std::vector<Difference> differences;
        size_t n_differences_elided = 0; // Differences beyond Options::detail_limit

        bool identical() const;
        bool equivalent() const; // Ignores winding, surface, and submesh differences
        bool comparable() const; // Face counts allowed a face by face comparison
    };

namespace detail
{

    //--------------------------------------------------------------------------
    // Face vertex loops grouped by surface and submesh, without brep::Mesh connectivity
    template<typename Traits, typename V = geometry::PositionNormalTexture<Traits>>
    class MeshDiffModel
    {
    public:

        using vector_traits = Traits;
        using value_type = Traits::value_type;
        using attributes_type = V;
        using point_type = V::point_type;
        using vector_type = V::vector_type;
        using texcoord_type = V::texcoord_type;
        using bounds_type = geometry::AxisAlignedBoundingBox<Traits>;

        static constexpr size_t dimension = Traits::dimension;

        //----------------------------------------------------------------------
        // An attribute the source did not supply is left unset, distinct from one with the default value
        class Vertex
        {
        public:

            point_type position;
            std::optional<vector_type> normal;
            std::optional<texcoord_type> texcoord;
        };

        //----------------------------------------------------------------------
        // A face's vertices occupy a contiguous range of the model vertex array
        class Face
        {
        public:

            id_type vertex_first = 0;
            size_t vertex_count = 0;
            id_type surface_id = 0;
        };

        //----------------------------------------------------------------------
        class Surface
        {
        public:

            std::string name;
            id_type submesh_id = 0;
            std::vector<id_type> face_ids;
        };

        //----------------------------------------------------------------------
        class Submesh
        {
        public:

            std::string name;
            std::vector<id_type> surface_ids;
        };

        //----------------------------------------------------------------------
        // The vertices of a face, modeling the point set interface of geometry::PositionAccessor so that normal,
        // vertex_centroid, bounds and area apply to it
        class FaceVertices
        {
        public:

            using value_type = MeshDiffModel::point_type;

            FaceVertices() = default;
            FaceVertices(const MeshDiffModel& model, id_type face_id);
            FaceVertices(const FaceVertices&) = default;
            ~FaceVertices() = default;

            FaceVertices& operator=(const FaceVertices&) = default;

            size_t size() const;
            value_type operator[](size_t n) const;

            const Vertex& vertex(size_t n) const;

        private:

            const MeshDiffModel* m_pmodel = nullptr;
            id_type m_vertex_first = 0;
            size_t m_vertex_count = 0;
        };

        using faces_type = std::vector<Face>;
        using surfaces_type = std::vector<Surface>;
        using submeshes_type = std::vector<Submesh>;
        using vertices_type = std::vector<Vertex>;

        MeshDiffModel() = default;
        MeshDiffModel(const MeshDiffModel&) = default;
        MeshDiffModel(MeshDiffModel&&) = default;
        ~MeshDiffModel() = default;

        MeshDiffModel& operator=(const MeshDiffModel&) = default;
        MeshDiffModel& operator=(MeshDiffModel&&) = default;

        // Construction, each returns the index of the new or existing element

        id_type get_submesh_id(const std::string& name);
        id_type get_surface_id(id_type submesh_id, const std::string& name);
        id_type create_face(id_type surface_id, id_type vertex_first, size_t vertex_count);
        void add_vertex(const Vertex& vertex);

        size_t face_count() const;
        size_t vertex_count() const;

        const vertices_type& vertices() const;
        const faces_type& faces() const;
        const surfaces_type& surfaces() const;
        const submeshes_type& submeshes() const;

        const Face& face(id_type face_id) const;
        FaceVertices face_vertices(id_type face_id) const;

        const std::string& surface_name(id_type face_id) const;
        const std::string& submesh_name(id_type face_id) const;

        // Lazy evaluation, invalidated by any subsequent construction
        const std::vector<point_type>& face_centroids() const;
        const point_type& face_centroid(id_type face_id) const;
        const bounds_type& bounds() const;

    private:

        void reset_cache();

        vertices_type m_vertices;
        faces_type m_faces;
        surfaces_type m_surfaces;
        submeshes_type m_submeshes;

        mutable std::optional<std::vector<point_type>> m_face_centroids;
        mutable std::optional<bounds_type> m_bounds;
    };

    template<typename M>
    using model_of = MeshDiffModel<typename M::vector_traits, typename M::vertex_attributes_type>;

    //--------------------------------------------------------------------------
    // The result of aligning two face vertex loops
    class FaceComparison
    {
    public:

        FaceDifferences differences;
        bool exact = false;    // Every compared attribute is bitwise equal
        bool reversed = false; // The loops align only in opposite directions
        size_t offset = 0;     // Rotation of the rhs loop that produced this alignment

        FaceMatch match() const;
    };

    //--------------------------------------------------------------------------
    // Read an obj file directly into a MeshDiffModel, bypassing brep::Mesh
    template<typename M>
    M read_obj_model(const std::filesystem::path& filepath);

    // Fill the equivalent MeshDiffModel from any brep::Mesh
    template<typename Mesh>
    model_of<Mesh> to_model(const Mesh& mesh);

    template<typename M>
    Comparison compare_models(const M& lhs, const M& rhs, const Options& options = {});

    // Align two vertex loops, nullopt when their vertex counts differ. A loop may be assembled from more than one face, as a merged triangulation is
    template<typename M, typename L, typename R>
    std::optional<FaceComparison> compare_loops(const L& vertices_lhs, const R& vertices_rhs, const Options& options);

    // Align two face vertex loops, nullopt when their vertex counts differ
    template<typename M>
    std::optional<FaceComparison> compare_faces(const M& lhs, id_type face_id_lhs, const M& rhs, id_type face_id_rhs, const Options& options);

    //--------------------------------------------------------------------------
    // A loop of vertices that need not be contiguous in the model, as in a loop merged from several faces. Read as a face's own vertices are
    template<typename M>
    class VertexLoop
    {
    public:

        using vertex_type = M::Vertex;
        using value_type = M::point_type;

        void push_back(const vertex_type& vertex) { m_vertices.push_back(&vertex); }

        size_t size() const { return m_vertices.size(); }
        value_type operator[](size_t n) const { return m_vertices[n]->position; }
        const vertex_type& vertex(size_t n) const { return *m_vertices[n]; }

    private:

        std::vector<const vertex_type*> m_vertices;
    };

    // The loop bounding two triangles that share an edge, that edge being the diagonal they were split
    // along, or nullopt where they are not two triangles sharing one
    template<typename M>
    std::optional<VertexLoop<M>> merge_triangles(const M& model, id_type face_id0, id_type face_id1, const Options& options);

    // Whether two triangles of a merged pair lie in one plane, in which case the diagonal between them
    // is arbitrary and the two triangulations describe the same surface
    template<typename M>
    bool coplanar_triangles(const M& model, id_type face_id0, id_type face_id1, const Options& options);

    //--------------------------------------------------------------------------
    // Uniform grid over face centroids, sized so that every position within
    // tolerance of a query lies in the cell of that query or one adjacent to it
    template<typename M>
    class CentroidGrid
    {
    public:

        using value_type = M::value_type;
        using point_type = M::point_type;

        static constexpr size_t dimension = M::dimension;
        static constexpr size_t cells_max = 256; // Per axis

        explicit CentroidGrid(const M& model, int tolerance);

        template<typename F>
        void for_each_candidate(const point_type& position, F f) const;

        value_type epsilon() const;

    private:

        using key_type = std::array<int64_t, dimension>;

        //----------------------------------------------------------------------
        struct KeyHash
        {
            size_t operator()(const key_type& key) const;
        };

        key_type key(const point_type& position) const;

        std::unordered_map<key_type, std::vector<id_type>, KeyHash> m_cells;
        value_type m_epsilon = value_type(0);
        value_type m_cell_size = value_type(1);
    };

    // The largest absolute difference that float_eq accepts at the given tolerance
    // anywhere within the bounds, used to size grid cells
    template<typename B>
    typename B::value_type absolute_epsilon(const B& bounds, int tolerance);

    // Two optional attributes agree when both are absent, or both are present and
    // equal within tolerance. An absent one never agrees with a present one
    template<typename T>
    bool attribute_eq(const std::optional<T>& lhs, const std::optional<T>& rhs, int tolerance);

} // namespace detail

} // namespace quetzal::model

//------------------------------------------------------------------------------
template<typename Traits, typename V>
quetzal::model::Comparison quetzal::model::mesh_diff(const std::filesystem::path& lhs, const std::filesystem::path& rhs, const Options& options)
{
    using model_type = detail::MeshDiffModel<Traits, V>;

    return detail::compare_models(detail::read_obj_model<model_type>(lhs), detail::read_obj_model<model_type>(rhs), options);
}

//------------------------------------------------------------------------------
template<quetzal::model::BrepMesh M>
quetzal::model::Comparison quetzal::model::mesh_diff(const M& lhs, const M& rhs, const Options& options)
{
    return detail::compare_models(detail::to_model(lhs), detail::to_model(rhs), options);
}

//------------------------------------------------------------------------------
template<quetzal::model::BrepMesh M>
quetzal::model::Comparison quetzal::model::mesh_diff(const std::filesystem::path& lhs, const M& rhs, const Options& options)
{
    // The file is read with the traits the mesh already supplies
    return detail::compare_models(detail::read_obj_model<detail::model_of<M>>(lhs), detail::to_model(rhs), options);
}

//------------------------------------------------------------------------------
template<quetzal::model::BrepMesh M>
quetzal::model::Comparison quetzal::model::mesh_diff(const M& lhs, const std::filesystem::path& rhs, const Options& options)
{
    return detail::compare_models(detail::to_model(lhs), detail::read_obj_model<detail::model_of<M>>(rhs), options);
}

//------------------------------------------------------------------------------
inline const std::string& quetzal::model::to_string(FaceMatch match)
{
    static const std::string identical = "identical";
    static const std::string equivalent = "equivalent";
    static const std::string reversed = "reversed";
    static const std::string different = "different";
    static const std::string unmatched = "unmatched";

    switch (match)
    {
        case FaceMatch::identical: return identical;
        case FaceMatch::equivalent: return equivalent;
        case FaceMatch::reversed: return reversed;
        case FaceMatch::different: return different;
        case FaceMatch::unmatched: break;
    }

    return unmatched;
}

//------------------------------------------------------------------------------
inline size_t quetzal::model::FaceDifferences::total() const
{
    return positions + normals + texcoords;
}

//------------------------------------------------------------------------------
inline bool quetzal::model::FaceDifferences::empty() const
{
    return total() == 0;
}

//------------------------------------------------------------------------------
inline bool quetzal::model::Comparison::identical() const
{
    return comparable() && n_identical == n_faces && n_surface_differences == 0;
}

//------------------------------------------------------------------------------
inline bool quetzal::model::Comparison::equivalent() const
{
    return comparable() && n_equivalent == n_faces;
}

//------------------------------------------------------------------------------
inline bool quetzal::model::Comparison::comparable() const
{
    return n_faces == n_faces_other && n_unmatched == 0 && n_unmatched_other == 0;
}

//------------------------------------------------------------------------------
template<typename Traits, typename V>
quetzal::model::detail::MeshDiffModel<Traits, V>::FaceVertices::FaceVertices(const MeshDiffModel& model, id_type face_id) :
    m_pmodel(&model),
    m_vertex_first(model.face(face_id).vertex_first),
    m_vertex_count(model.face(face_id).vertex_count)
{
}

//------------------------------------------------------------------------------
template<typename Traits, typename V>
size_t quetzal::model::detail::MeshDiffModel<Traits, V>::FaceVertices::size() const
{
    return m_vertex_count;
}

//------------------------------------------------------------------------------
template<typename Traits, typename V>
typename quetzal::model::detail::MeshDiffModel<Traits, V>::FaceVertices::value_type quetzal::model::detail::MeshDiffModel<Traits, V>::FaceVertices::operator[](size_t n) const
{
    return vertex(n).position;
}

//------------------------------------------------------------------------------
template<typename Traits, typename V>
const typename quetzal::model::detail::MeshDiffModel<Traits, V>::Vertex& quetzal::model::detail::MeshDiffModel<Traits, V>::FaceVertices::vertex(size_t n) const
{
    assert(m_pmodel != nullptr);
    assert(n < m_vertex_count);
    return m_pmodel->vertices()[m_vertex_first + n];
}

//------------------------------------------------------------------------------
template<typename Traits, typename V>
quetzal::id_type quetzal::model::detail::MeshDiffModel<Traits, V>::get_submesh_id(const std::string& name)
{
    auto i = std::ranges::find_if(m_submeshes, [&name](const Submesh& submesh) { return submesh.name == name; });
    if (i != m_submeshes.end())
    {
        return static_cast<id_type>(i - m_submeshes.begin());
    }

    m_submeshes.push_back(Submesh{name, {}});
    return m_submeshes.size() - 1;
}

//------------------------------------------------------------------------------
template<typename Traits, typename V>
quetzal::id_type quetzal::model::detail::MeshDiffModel<Traits, V>::get_surface_id(id_type submesh_id, const std::string& name)
{
    assert(submesh_id < m_submeshes.size());

    Submesh& submesh = m_submeshes[submesh_id];
    for (id_type surface_id : submesh.surface_ids)
    {
        if (m_surfaces[surface_id].name == name)
        {
            return surface_id;
        }
    }

    m_surfaces.push_back(Surface{name, submesh_id, {}});
    submesh.surface_ids.push_back(m_surfaces.size() - 1);
    return m_surfaces.size() - 1;
}

//------------------------------------------------------------------------------
template<typename Traits, typename V>
quetzal::id_type quetzal::model::detail::MeshDiffModel<Traits, V>::create_face(id_type surface_id, id_type vertex_first, size_t vertex_count)
{
    assert(surface_id < m_surfaces.size());
    assert(vertex_first + vertex_count <= m_vertices.size());

    m_faces.push_back(Face{vertex_first, vertex_count, surface_id});
    m_surfaces[surface_id].face_ids.push_back(m_faces.size() - 1);

    reset_cache();
    return m_faces.size() - 1;
}

//------------------------------------------------------------------------------
template<typename Traits, typename V>
void quetzal::model::detail::MeshDiffModel<Traits, V>::add_vertex(const Vertex& vertex)
{
    m_vertices.push_back(vertex);
    m_bounds.reset();
    return;
}

//------------------------------------------------------------------------------
template<typename Traits, typename V>
size_t quetzal::model::detail::MeshDiffModel<Traits, V>::face_count() const
{
    return m_faces.size();
}

//------------------------------------------------------------------------------
template<typename Traits, typename V>
size_t quetzal::model::detail::MeshDiffModel<Traits, V>::vertex_count() const
{
    return m_vertices.size();
}

//------------------------------------------------------------------------------
template<typename Traits, typename V>
const typename quetzal::model::detail::MeshDiffModel<Traits, V>::vertices_type& quetzal::model::detail::MeshDiffModel<Traits, V>::vertices() const
{
    return m_vertices;
}

//------------------------------------------------------------------------------
template<typename Traits, typename V>
const typename quetzal::model::detail::MeshDiffModel<Traits, V>::faces_type& quetzal::model::detail::MeshDiffModel<Traits, V>::faces() const
{
    return m_faces;
}

//------------------------------------------------------------------------------
template<typename Traits, typename V>
const typename quetzal::model::detail::MeshDiffModel<Traits, V>::surfaces_type& quetzal::model::detail::MeshDiffModel<Traits, V>::surfaces() const
{
    return m_surfaces;
}

//------------------------------------------------------------------------------
template<typename Traits, typename V>
const typename quetzal::model::detail::MeshDiffModel<Traits, V>::submeshes_type& quetzal::model::detail::MeshDiffModel<Traits, V>::submeshes() const
{
    return m_submeshes;
}

//------------------------------------------------------------------------------
template<typename Traits, typename V>
const typename quetzal::model::detail::MeshDiffModel<Traits, V>::Face& quetzal::model::detail::MeshDiffModel<Traits, V>::face(id_type face_id) const
{
    assert(face_id < m_faces.size());
    return m_faces[face_id];
}

//------------------------------------------------------------------------------
template<typename Traits, typename V>
typename quetzal::model::detail::MeshDiffModel<Traits, V>::FaceVertices quetzal::model::detail::MeshDiffModel<Traits, V>::face_vertices(id_type face_id) const
{
    return FaceVertices(*this, face_id);
}

//------------------------------------------------------------------------------
template<typename Traits, typename V>
const std::string& quetzal::model::detail::MeshDiffModel<Traits, V>::surface_name(id_type face_id) const
{
    return m_surfaces[face(face_id).surface_id].name;
}

//------------------------------------------------------------------------------
template<typename Traits, typename V>
const std::string& quetzal::model::detail::MeshDiffModel<Traits, V>::submesh_name(id_type face_id) const
{
    return m_submeshes[m_surfaces[face(face_id).surface_id].submesh_id].name;
}

//------------------------------------------------------------------------------
template<typename Traits, typename V>
const std::vector<typename quetzal::model::detail::MeshDiffModel<Traits, V>::point_type>& quetzal::model::detail::MeshDiffModel<Traits, V>::face_centroids() const
{
    if (!m_face_centroids)
    {
        std::vector<point_type> centroids;
        centroids.reserve(m_faces.size());

        for (id_type face_id = 0; face_id < m_faces.size(); ++face_id)
        {
            centroids.push_back(geometry::vertex_centroid(face_vertices(face_id)));
        }

        m_face_centroids = std::move(centroids);
    }

    return *m_face_centroids;
}

//------------------------------------------------------------------------------
template<typename Traits, typename V>
const typename quetzal::model::detail::MeshDiffModel<Traits, V>::point_type& quetzal::model::detail::MeshDiffModel<Traits, V>::face_centroid(id_type face_id) const
{
    assert(face_id < m_faces.size());
    return face_centroids()[face_id];
}

//------------------------------------------------------------------------------
template<typename Traits, typename V>
const typename quetzal::model::detail::MeshDiffModel<Traits, V>::bounds_type& quetzal::model::detail::MeshDiffModel<Traits, V>::bounds() const
{
    if (!m_bounds)
    {
        bounds_type b;
        for (const Vertex& vertex : m_vertices)
        {
            b.insert(vertex.position);
        }

        m_bounds = b;
    }

    return *m_bounds;
}

//------------------------------------------------------------------------------
template<typename Traits, typename V>
void quetzal::model::detail::MeshDiffModel<Traits, V>::reset_cache()
{
    m_face_centroids.reset();
    m_bounds.reset();
    return;
}

//------------------------------------------------------------------------------
inline quetzal::model::FaceMatch quetzal::model::detail::FaceComparison::match() const
{
    if (!differences.empty())
    {
        return FaceMatch::different;
    }

    if (reversed)
    {
        return FaceMatch::reversed;
    }

    return exact ? FaceMatch::identical : FaceMatch::equivalent;
}

//------------------------------------------------------------------------------
template<typename M>
M quetzal::model::detail::read_obj_model(const std::filesystem::path& filepath)
{
    using attributes_type = M::attributes_type;
    using point_type = M::point_type;
    using vector_type = M::vector_type;
    using texcoord_type = M::texcoord_type;

    M model;

    id_type submesh_id = nullid;
    id_type surface_id = nullid;
    id_type vertex_first = 0;

    auto on_group = [&](M& model, const std::string& name) -> void
    {
        if (submesh_id == nullid)
        {
            submesh_id = model.get_submesh_id(wavefront_obj::ObjectNameDefault);
        }

        surface_id = model.get_surface_id(submesh_id, name);
        return;
    };

    auto on_face_open = [&](M& model) -> void
    {
        if (surface_id == nullid)
        {
            on_group(model, wavefront_obj::GroupNameDefault);
        }

        vertex_first = model.vertex_count();
        return;
    };

    auto on_face_vertex = [&](M& model, const point_type& position, const std::optional<vector_type>& normal, const std::optional<texcoord_type>& texcoord) -> void
    {
        // An attribute the file omits stays unset
        model.add_vertex({position, normal, texcoord});
        return;
    };

    auto on_face_close = [&](M& model) -> void
    {
        model.create_face(surface_id, vertex_first, model.vertex_count() - vertex_first);
        return;
    };

    wavefront_obj::Reader<M> reader(
        [](M&, const std::filesystem::path&) -> void {},
        [](M&, const point_type&) -> void {},
        [](M&, const vector_type&) -> void {},
        [](M&, const texcoord_type&) -> void {},
        [&](M& model, const std::string& name) -> void { submesh_id = model.get_submesh_id(name); surface_id = nullid; },
        on_group,
        on_face_open,
        on_face_vertex,
        on_face_close,
        [](M&, const std::string&) -> void {},
        [](M&, const std::string&) -> void {});

    reader.read(model, filepath);
    return model;
}

//------------------------------------------------------------------------------
template<typename Mesh>
quetzal::model::detail::model_of<Mesh> quetzal::model::detail::to_model(const Mesh& mesh)
{
    using model_type = MeshDiffModel<typename Mesh::vector_traits, typename Mesh::vertex_attributes_type>;

    model_type model;

    for (const auto& face : mesh.faces())
    {
        const std::string& submesh_name = face.submesh_id() == nullid ? wavefront_obj::ObjectNameDefault : face.submesh().name();
        const std::string& surface_name = face.surface_id() == nullid ? wavefront_obj::GroupNameDefault : face.surface().name();

        id_type submesh_id = model.get_submesh_id(submesh_name);
        id_type surface_id = model.get_surface_id(submesh_id, surface_name);

        id_type vertex_first = model.vertex_count();
        for (const auto& halfedge : face.halfedges())
        {
            using attributes_type = model_type::attributes_type;

            // A mesh vertex always has the attributes its type provides
            const auto& av = halfedge.vertex_attributes();

            typename model_type::Vertex vertex;
            vertex.position = av.position();

            if constexpr (attributes_type::contains(geometry::AttributesFlags::Normal))
            {
                vertex.normal = av.normal();
            }

            if constexpr (attributes_type::contains(geometry::AttributesFlags::Texcoord0))
            {
                vertex.texcoord = av.texcoord();
            }

            model.add_vertex(vertex);
        }

        model.create_face(surface_id, vertex_first, model.vertex_count() - vertex_first);
    }

    return model;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::model::Comparison quetzal::model::detail::compare_models(const M& lhs, const M& rhs, const Options& options)
{
    Comparison comparison;
    comparison.n_faces = lhs.face_count();
    comparison.n_faces_other = rhs.face_count();

    auto qualified_name = [](const M& model, id_type face_id) -> std::string
    {
        return model.submesh_name(face_id) + "/" + model.surface_name(face_id);
    };

    auto record = [&](const Difference& difference) -> void
    {
        if (options.detail_limit == 0 || comparison.differences.size() < options.detail_limit)
        {
            comparison.differences.push_back(difference);
            return;
        }

        ++comparison.n_differences_elided;
        return;
    };

    CentroidGrid<M> grid(rhs, options.tolerance);
    std::vector<bool> consumed(rhs.face_count(), false);

    std::vector<id_type> unmatched_lhs;
    std::vector<id_type> unmatched_rhs;

    for (id_type face_id = 0; face_id < lhs.face_count(); ++face_id)
    {
        const auto& centroid = lhs.face_centroid(face_id);

        std::optional<FaceComparison> best;
        id_type best_id = 0;

        grid.for_each_candidate(centroid, [&](id_type candidate) -> bool
        {
            if (consumed[candidate] || !float_eq(centroid, rhs.face_centroid(candidate), options.tolerance))
            {
                return true;
            }

            auto result = compare_faces(lhs, face_id, rhs, candidate, options);
            if (!result)
            {
                return true;
            }

            if (!best || result->differences.total() < best->differences.total() || (result->differences.total() == best->differences.total() && !result->reversed && best->reversed))
            {
                best = result;
                best_id = candidate;
            }

            // Nothing can improve on an exact forward match
            return !(best->differences.empty() && best->exact && !best->reversed);
        });

        if (!best)
        {
            unmatched_lhs.push_back(face_id);
            continue;
        }

        consumed[best_id] = true;

        FaceMatch match = best->match();
        switch (match)
        {
            case FaceMatch::identical:
                ++comparison.n_identical;
                ++comparison.n_equivalent;
                break;

            case FaceMatch::equivalent:
                ++comparison.n_equivalent;
                break;

            case FaceMatch::reversed:
                ++comparison.n_reversed;
                break;

            default:
                ++comparison.n_different;
                break;
        }

        std::string name_lhs = qualified_name(lhs, face_id);
        std::string name_rhs = qualified_name(rhs, best_id);
        bool surface_differs = options.compare_surfaces && name_lhs != name_rhs;
        if (surface_differs)
        {
            ++comparison.n_surface_differences;
        }

        if (match != FaceMatch::identical || surface_differs)
        {
            record({face_id, best_id, match, best->differences, name_lhs, name_rhs});
        }
    }

    for (id_type face_id = 0; face_id < rhs.face_count(); ++face_id)
    {
        if (!consumed[face_id])
        {
            unmatched_rhs.push_back(face_id);
        }
    }

    // A polygon triangulated along different diagonals on the two sides leaves its triangles unmatched on both;
    // merging each pair back into its polygon matches them
    std::vector<bool> merged_lhs(unmatched_lhs.size(), false);
    std::vector<bool> merged_rhs(unmatched_rhs.size(), false);

    for (size_t a0 = 0; a0 < unmatched_lhs.size(); ++a0)
    {
        for (size_t a1 = a0 + 1; a1 < unmatched_lhs.size() && !merged_lhs[a0]; ++a1)
        {
            if (merged_lhs[a1])
            {
                continue;
            }

            auto loop_lhs = merge_triangles(lhs, unmatched_lhs[a0], unmatched_lhs[a1], options);
            if (!loop_lhs)
            {
                continue;
            }

            for (size_t b0 = 0; b0 < unmatched_rhs.size() && !merged_lhs[a0]; ++b0)
            {
                for (size_t b1 = b0 + 1; b1 < unmatched_rhs.size() && !merged_lhs[a0]; ++b1)
                {
                    if (merged_rhs[b0] || merged_rhs[b1])
                    {
                        continue;
                    }

                    auto loop_rhs = merge_triangles(rhs, unmatched_rhs[b0], unmatched_rhs[b1], options);
                    if (!loop_rhs)
                    {
                        continue;
                    }

                    auto result = compare_loops<M>(*loop_lhs, *loop_rhs, options);
                    if (!result || !result->differences.empty())
                    {
                        continue;
                    }

                    merged_lhs[a0] = true;
                    merged_lhs[a1] = true;
                    merged_rhs[b0] = true;
                    merged_rhs[b1] = true;

                    // The two triangulations describe one surface only across a planar polygon
                    bool planar = coplanar_triangles(lhs, unmatched_lhs[a0], unmatched_lhs[a1], options)
                        && coplanar_triangles(rhs, unmatched_rhs[b0], unmatched_rhs[b1], options);

                    comparison.n_triangulation += planar ? 2 : 0;
                    comparison.n_triangulation_nonplanar += planar ? 0 : 2;

                    for (size_t i = 0; i < 2; ++i)
                    {
                        id_type face_id_lhs = i == 0 ? unmatched_lhs[a0] : unmatched_lhs[a1];
                        id_type face_id_rhs = i == 0 ? unmatched_rhs[b0] : unmatched_rhs[b1];

                        if (planar)
                        {
                            ++comparison.n_equivalent;
                        }
                        else
                        {
                            ++comparison.n_different;
                        }

                        record({face_id_lhs, face_id_rhs, planar ? FaceMatch::equivalent : FaceMatch::different, {}, qualified_name(lhs, face_id_lhs), qualified_name(rhs, face_id_rhs)});
                    }
                }
            }
        }
    }

    for (size_t i = 0; i < unmatched_lhs.size(); ++i)
    {
        if (!merged_lhs[i])
        {
            ++comparison.n_unmatched;
            record({unmatched_lhs[i], std::nullopt, FaceMatch::unmatched, {}, qualified_name(lhs, unmatched_lhs[i]), {}});
        }
    }

    for (size_t i = 0; i < unmatched_rhs.size(); ++i)
    {
        if (!merged_rhs[i])
        {
            ++comparison.n_unmatched_other;
            record({unmatched_rhs[i], std::nullopt, FaceMatch::unmatched, {}, {}, qualified_name(rhs, unmatched_rhs[i])});
        }
    }

    return comparison;
}

//------------------------------------------------------------------------------
template<typename M>
std::optional<quetzal::model::detail::FaceComparison> quetzal::model::detail::compare_faces(const M& lhs, id_type face_id_lhs, const M& rhs, id_type face_id_rhs, const Options& options)
{
    return compare_loops<M>(lhs.face_vertices(face_id_lhs), rhs.face_vertices(face_id_rhs), options);
}

//------------------------------------------------------------------------------
template<typename M>
std::optional<quetzal::model::detail::VertexLoop<M>> quetzal::model::detail::merge_triangles(const M& model, id_type face_id0, id_type face_id1, const Options& options)
{
    auto vertices0 = model.face_vertices(face_id0);
    auto vertices1 = model.face_vertices(face_id1);

    if (vertices0.size() != 3 || vertices1.size() != 3)
    {
        return std::nullopt;
    }

    auto position_index = [&](const auto& position) -> int
    {
        for (int i = 0; i < 3; ++i)
        {
            if (float_eq(vertices1[i], position, options.tolerance))
            {
                return i;
            }
        }

        return -1;
    };

    for (size_t i = 0; i < 3; ++i)
    {
        size_t j = (i + 1) % 3;

        int i_shared = position_index(vertices0[i]);
        int j_shared = position_index(vertices0[j]);
        if (i_shared < 0 || j_shared < 0)
        {
            continue;
        }

        // The one vertex of the second triangle that is not on the shared edge, taking the place of
        // that edge in the first triangle's loop
        VertexLoop<M> loop;
        loop.push_back(vertices0.vertex((i + 2) % 3));
        loop.push_back(vertices0.vertex(i));
        loop.push_back(vertices1.vertex(3 - i_shared - j_shared));
        loop.push_back(vertices0.vertex(j));
        return loop;
    }

    return std::nullopt;
}

//------------------------------------------------------------------------------
template<typename M>
bool quetzal::model::detail::coplanar_triangles(const M& model, id_type face_id0, id_type face_id1, const Options& options)
{
    auto vertices0 = model.face_vertices(face_id0);
    auto vertices1 = model.face_vertices(face_id1);

    auto normal_of = [](const auto& vertices)
    {
        return normalize(cross(vertices[1] - vertices[0], vertices[2] - vertices[0]));
    };

    return float_eq(normal_of(vertices0), normal_of(vertices1), options.tolerance);
}

//------------------------------------------------------------------------------
template<typename M, typename L, typename R>
std::optional<quetzal::model::detail::FaceComparison> quetzal::model::detail::compare_loops(const L& vertices_lhs, const R& vertices_rhs, const Options& options)
{
    using attributes_type = M::attributes_type;

    size_t n = vertices_lhs.size();
    if (n != vertices_rhs.size())
    {
        return std::nullopt;
    }

    if (n == 0)
    {
        FaceComparison comparison;
        comparison.exact = true;
        return comparison;
    }

    constexpr bool has_normals = attributes_type::contains(geometry::AttributesFlags::Normal);
    constexpr bool has_texcoords = attributes_type::contains(geometry::AttributesFlags::Texcoord0);

    // Differences between the lhs loop and the rhs loop entered at offset and
    // traversed in the given direction
    auto align = [&](size_t offset, bool reversed) -> FaceComparison
    {
        FaceComparison comparison;
        comparison.offset = offset;
        comparison.reversed = reversed;
        comparison.exact = true;

        for (size_t i = 0; i < n; ++i)
        {
            size_t j = reversed ? (offset + n - i) % n : (offset + i) % n;

            const auto& vertex_lhs = vertices_lhs.vertex(i);
            const auto& vertex_rhs = vertices_rhs.vertex(j);

            if (vertex_lhs.position != vertex_rhs.position)
            {
                comparison.exact = false;
                if (!float_eq(vertex_lhs.position, vertex_rhs.position, options.tolerance))
                {
                    ++comparison.differences.positions;
                }
            }

            if constexpr (has_normals)
            {
                if (options.compare_normals && vertex_lhs.normal != vertex_rhs.normal)
                {
                    comparison.exact = false;
                    if (!attribute_eq(vertex_lhs.normal, vertex_rhs.normal, options.tolerance))
                    {
                        ++comparison.differences.normals;
                    }
                }
            }

            if constexpr (has_texcoords)
            {
                if (options.compare_texcoords && vertex_lhs.texcoord != vertex_rhs.texcoord)
                {
                    comparison.exact = false;
                    if (!attribute_eq(vertex_lhs.texcoord, vertex_rhs.texcoord, options.tolerance))
                    {
                        ++comparison.differences.texcoords;
                    }
                }
            }
        }

        return comparison;
    };

    // A better alignment has fewer differing vertices, and at equal counts is
    // exact rather than approximate and forward rather than reversed
    auto better = [](const FaceComparison& a, const FaceComparison& b) -> bool
    {
        if (a.differences.total() != b.differences.total())
        {
            return a.differences.total() < b.differences.total();
        }

        if (a.reversed != b.reversed)
        {
            return !a.reversed;
        }

        return a.exact && !b.exact;
    };

    std::optional<FaceComparison> best;

    for (bool reversed : {false, true})
    {
        for (size_t offset = 0; offset < n; ++offset)
        {
            // Only alignments that put a matching vertex first are worth walking
            if (!float_eq(vertices_lhs[0], vertices_rhs[offset], options.tolerance))
            {
                continue;
            }

            FaceComparison comparison = align(offset, reversed);
            if (!best || better(comparison, *best))
            {
                best = comparison;
            }

            if (best->differences.empty() && best->exact && !best->reversed)
            {
                return best;
            }
        }

        if (best && best->differences.empty())
        {
            return best;
        }
    }

    // No vertex of the rhs loop corresponds to the first vertex of the lhs loop
    if (!best)
    {
        best = align(0, false);
    }

    return best;
}

//------------------------------------------------------------------------------
template<typename M>
size_t quetzal::model::detail::CentroidGrid<M>::KeyHash::operator()(const key_type& key) const
{
    size_t h = 14695981039346656037ull;
    for (int64_t i : key)
    {
        h = (h ^ static_cast<size_t>(i)) * 1099511628211ull;
    }

    return h;
}

//------------------------------------------------------------------------------
template<typename M>
quetzal::model::detail::CentroidGrid<M>::CentroidGrid(const M& model, int tolerance)
{
    m_epsilon = absolute_epsilon(model.bounds(), tolerance);

    // Cells at least two epsilon across so that a tolerance neighborhood never
    // spans more than the adjacent cell on either side of an axis, and no more
    // than cells_max per axis so that occupancy stays useful for large models
    value_type extent = value_type(0);
    if (!model.bounds().empty())
    {
        for (size_t i = 0; i < dimension; ++i)
        {
            extent = std::max(extent, model.bounds().extent(i));
        }
    }

    m_cell_size = std::max({value_type(2) * m_epsilon, extent / value_type(cells_max), std::numeric_limits<value_type>::min()});

    const auto& centroids = model.face_centroids();
    for (id_type face_id = 0; face_id < centroids.size(); ++face_id)
    {
        m_cells[key(centroids[face_id])].push_back(face_id);
    }
}

//------------------------------------------------------------------------------
template<typename M>
typename quetzal::model::detail::CentroidGrid<M>::key_type quetzal::model::detail::CentroidGrid<M>::key(const point_type& position) const
{
    key_type k;
    for (size_t i = 0; i < dimension; ++i)
    {
        k[i] = static_cast<int64_t>(std::floor(position[i] / m_cell_size));
    }

    return k;
}

//------------------------------------------------------------------------------
template<typename M>
template<typename F>
void quetzal::model::detail::CentroidGrid<M>::for_each_candidate(const point_type& position, F f) const
{
    key_type k = key(position);

    // Every cell in the surrounding 3^dimension neighborhood
    std::array<int64_t, dimension> offsets{};
    offsets.fill(-1);

    for (;;)
    {
        key_type neighbor;
        for (size_t i = 0; i < dimension; ++i)
        {
            neighbor[i] = k[i] + offsets[i];
        }

        auto i = m_cells.find(neighbor);
        if (i != m_cells.end())
        {
            for (id_type face_id : i->second)
            {
                if (!f(face_id))
                {
                    return;
                }
            }
        }

        size_t axis = 0;
        for (; axis < dimension; ++axis)
        {
            if (++offsets[axis] <= 1)
            {
                break;
            }

            offsets[axis] = -1;
        }

        if (axis == dimension)
        {
            break;
        }
    }

    return;
}

//------------------------------------------------------------------------------
template<typename M>
typename quetzal::model::detail::CentroidGrid<M>::value_type quetzal::model::detail::CentroidGrid<M>::epsilon() const
{
    return m_epsilon;
}

//------------------------------------------------------------------------------
template<typename B>
typename B::value_type quetzal::model::detail::absolute_epsilon(const B& bounds, int tolerance)
{
    using value_type = B::value_type;

    value_type scale = value_type(1);
    if (!bounds.empty())
    {
        for (size_t i = 0; i < B::point_type::dimension; ++i)
        {
            scale = std::max({scale, std::abs(bounds.lower()[i]), std::abs(bounds.upper()[i])});
        }
    }

    return std::numeric_limits<value_type>::epsilon() * static_cast<value_type>(tolerance) * scale;
}

//------------------------------------------------------------------------------
template<typename T>
bool quetzal::model::detail::attribute_eq(const std::optional<T>& lhs, const std::optional<T>& rhs, int tolerance)
{
    if (!lhs || !rhs)
    {
        return !lhs && !rhs;
    }

    return float_eq(*lhs, *rhs, tolerance);
}

#endif // QUETZAL_MODEL_MESH_DIFF_HPP
