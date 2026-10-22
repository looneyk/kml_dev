#if !defined(QUETZAL_WAVEFRONT_OBJ_READER_HPP)
#define QUETZAL_WAVEFRONT_OBJ_READER_HPP
//------------------------------------------------------------------------------
// Wavefront obj file i/o
// Reader.hpp
//------------------------------------------------------------------------------

#include "quetzal/common/Exception.hpp"
#include "quetzal/common/file_util.hpp"
#include "reader_util.hpp"
#include "symbols.hpp"
#include <array>
#include <filesystem>
#include <functional>
#include <fstream>
#include <iostream> // warning messages, change to log ...
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <type_traits>

namespace quetzal::wavefront_obj
{

    template <typename T>
    concept HasTexcoordType = requires { typename T::texcoord_type; };

    template <typename T>
    concept HasVertexAttributesTexcoordType = requires { typename T::vertex_attributes_type::texcoord_type; };

    template<typename T>
    struct t_type
    {
        using type = void;
    };

    template<HasTexcoordType T>
    struct t_type<T>
    {
        using type = T::texcoord_type;
    };

    template<typename T> requires (!HasTexcoordType<T> && HasVertexAttributesTexcoordType<T>)
    struct t_type<T>
    {
        using type = T::vertex_attributes_type::texcoord_type;
    };

    template<typename T>
    using t_type_t = typename t_type<T>::type;

    //--------------------------------------------------------------------------
    template<typename M>
    class Reader
    {
    public:

        using value_type = M::value_type;
        using point_type = M::point_type;
        using vector_type = M::vector_type;
        using texcoord_type = t_type_t<M>;

        using open_function_type = std::function<void(M&, const std::filesystem::path&)>;
        using position_function_type = std::function<void(M&, const point_type&)>;
        using normal_function_type = std::function<void(M&, const vector_type&)>;
        using texcoord_function_type = std::function<void(M&, const texcoord_type&)>;
        using object_function_type = std::function<void(M&, const std::string&)>;
        using group_function_type = std::function<void(M&, const std::string&)>;
        using face_open_function_type = std::function<void(M&)>;
        using face_vertex_function_type = std::function<void(M&, const point_type&, const std::optional<vector_type>&, const std::optional<texcoord_type>&)>;
        using face_close_function_type = std::function<void(M&)>;
        using materials_function_type = std::function<void(M&, const std::string&)>;
        using material_function_type = std::function<void(M&, const std::string&)>;

        Reader(open_function_type on_open, position_function_type on_position, normal_function_type on_normal, texcoord_function_type on_texcoord, object_function_type on_object, group_function_type on_group, face_open_function_type on_face_open, face_vertex_function_type on_face_vertex, face_close_function_type on_face_close, materials_function_type on_materials, material_function_type on_material);
        Reader(const Reader&) = delete;
        Reader(Reader&&) = delete;
        ~Reader() = default;

        Reader& operator=(const Reader&) = delete;
        Reader& operator=(Reader&&) = delete;

        void read(M& m, const std::filesystem::path& filepath) const;

    private:

        open_function_type m_on_open;
        position_function_type m_on_position;
        normal_function_type m_on_normal;
        texcoord_function_type m_on_texcoord;
        object_function_type m_on_object;
        group_function_type m_on_group;
        face_open_function_type m_on_face_open;
        face_vertex_function_type m_on_face_vertex;
        face_close_function_type m_on_face_close;
        materials_function_type m_on_materials;
        material_function_type m_on_material;
    };

} // namespace quetzal::wavefront_obj

//------------------------------------------------------------------------------
template<typename M>
quetzal::wavefront_obj::Reader<M>::Reader(open_function_type on_open, position_function_type on_position, normal_function_type on_normal, texcoord_function_type on_texcoord, object_function_type on_object, group_function_type on_group, face_open_function_type on_face_open, face_vertex_function_type on_face_vertex, face_close_function_type on_face_close, materials_function_type on_materials, material_function_type on_material) :
    m_on_open(on_open),
    m_on_position(on_position),
    m_on_normal(on_normal),
    m_on_texcoord(on_texcoord),
    m_on_object(on_object),
    m_on_group(on_group),
    m_on_face_open(on_face_open),
    m_on_face_vertex(on_face_vertex),
    m_on_face_close(on_face_close),
    m_on_materials(on_materials),
    m_on_material(on_material)
{
}

//------------------------------------------------------------------------------
template<typename M>
void quetzal::wavefront_obj::Reader<M>::read(M& m, const std::filesystem::path& filepath) const
{
    std::ifstream is(filepath, std::ios_base::binary);
    if (!is)
    {
        std::ostringstream oss;
        oss << "Error opening input file " << filepath;
        throw Exception(oss.str(), __FILE__, __LINE__);
    }

    m_on_open(m, filepath);

    std::vector<point_type> positions;
    std::vector<vector_type> normals;
    std::vector<texcoord_type> texcoords;
    size_t n = 0;

    // Reused across lines so that reading a file does not allocate per line
    std::string line;
    std::vector<std::string_view> tokens;

    while (!is.eof())
    {
        read_line(is, line);
        ++n;

        tokenize(line, tokens);
        if (tokens.empty())
        {
            continue;
        }

        std::string_view keyword = tokens[0];

        if (keyword == wavefront_obj::Keyword::Position)
        {
            std::array<value_type, 4> values;
            bool b = parse_components(tokens, values, 3, value_type(1)); // w = 1 for homogeneous coordinates
            if (!b)
            {
                std::ostringstream oss;
                oss << "Error reading vertex position in file " << filepath << " line " << n;
                throw Exception(oss.str(), __FILE__, __LINE__);
            }

            point_type position(values[0], values[1], values[2], values[3]);
            positions.emplace_back(position);

            m_on_position(m, position);
        }
        else if (keyword == wavefront_obj::Keyword::Normal)
        {
            std::array<value_type, 3> values;
            bool b = parse_components(tokens, values, 3);
            if (!b)
            {
                std::ostringstream oss;
                oss << "Error reading vertex normal in file " << filepath << " line " << n;
                throw Exception(oss.str(), __FILE__, __LINE__);
            }

            vector_type normal(values[0], values[1], values[2]);
            normals.emplace_back(normal);

            m_on_normal(m, normal);
        }
        else if (keyword == wavefront_obj::Keyword::Texcoord)
        {
            std::array<value_type, 3> values;
            bool b = parse_components(tokens, values, 1);
            if (!b)
            {
                std::ostringstream oss;
                oss << "Error reading texture coordinates in file " << filepath << " line " << n;
                throw Exception(oss.str(), __FILE__, __LINE__);
            }

            // 3D texture coordinates are truncated
            texcoord_type texcoord(values[0], values[1], value_type(1));
            texcoords.emplace_back(texcoord);

            m_on_texcoord(m, texcoord);
        }
        else if (keyword == wavefront_obj::Keyword::Face)
        {
            if (tokens.size() < 4)
            {
                std::ostringstream oss;
                oss << "Error reading face in file " << filepath << " line " << n;
                throw Exception(oss.str(), __FILE__, __LINE__);
            }

            m_on_face_open(m);

            for (size_t i = 1; i < tokens.size(); ++i)
            {
                std::string_view s = tokens[i];

                vertex_reference_type vr;
                bool b = parse_vertex_reference(s, vr);
                if (!b)
                {
                    std::ostringstream oss;
                    oss << "Error reading vertex reference in file " << filepath << " line " << n;
                    throw Exception(oss.str(), __FILE__, __LINE__);
                }

                point_type position = positions[vr[0] - 1];

                std::optional<vector_type> normal;
                if (vr[2] > 0)
                {
                    normal = normals[vr[2] - 1];
                }

                std::optional<texcoord_type> texcoord;
                if (vr[1] > 0)
                {
                    texcoord = texcoords[vr[1] - 1];
                }

                m_on_face_vertex(m, position, normal, texcoord);
            }

            m_on_face_close(m);
        }
        else if (keyword == wavefront_obj::Keyword::Group)
        {
            if (tokens.size() < 2)
            {
                std::cout << "Warning: No parameter for " << wavefront_obj::Keyword::Group << " keyword: " << (tokens.size() - 1) << " found, at least 1 expected." << std::endl;
            }

            m_on_group(m, tokens.size() > 1 && !tokens[1].empty() ? std::string(tokens[1]) : GroupNameDefault);
        }
        else if (keyword == wavefront_obj::Keyword::Object)
        {
            if (tokens.size() != 2)
            {
                std::cout << "Warning: Wrong number of parameters for " << wavefront_obj::Keyword::Object << " keyword: " << (tokens.size() - 1) << " found, 1 expected." << std::endl;
            }

            m_on_object(m, std::string(tokens[1]));
        }
        else if (keyword == wavefront_obj::Keyword::Materials)
        {
            if (tokens.size() != 2)
            {
                std::cout << "Warning: Wrong number of parameters for " << wavefront_obj::Keyword::Materials << " keyword: " << (tokens.size() - 1) << " found, 1 expected." << std::endl;
            }

            m_on_materials(m, std::string(tokens[1]));
        }
        else if (keyword == wavefront_obj::Keyword::Material)
        {
            if (tokens.size() != 2)
            {
                std::cout << "Warning: Wrong number of parameters for " << wavefront_obj::Keyword::Material << " keyword: " << (tokens.size() - 1) << " found, 1 expected." << std::endl;
            }

            m_on_material(m, std::string(tokens[1]));
        }

        // Comments, missing, unrecognized, and all other keywords ignored ...
    }
}

#endif // QUETZAL_WAVEFRONT_OBJ_READER_HPP
