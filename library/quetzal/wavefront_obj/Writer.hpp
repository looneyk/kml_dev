#if !defined(QUETZAL_WAVEFRONT_OBJ_WRITER_HPP)
#define QUETZAL_WAVEFRONT_OBJ_WRITER_HPP
//------------------------------------------------------------------------------
// Wavefront obj file i/o
// Writer.hpp
//------------------------------------------------------------------------------

#include "quetzal/common/Exception.hpp"
#include "quetzal/math/vector_util.hpp"
#include "quetzal/geometry/point_util.hpp"
#include "symbols.hpp"
#include <map>
#include <vector>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <cassert>

namespace quetzal::wavefront_obj
{

    //--------------------------------------------------------------------------
    template<typename P, typename V, typename T>
    class Writer
    {
    public:

        using point_type = P;
        using vector_type = V;
        using texcoord_type = T;

        Writer(const std::filesystem::path& filepath);
        Writer(const Writer&) = delete;
        Writer(Writer&&) = delete;
        ~Writer();

        Writer& operator=(const Writer&) = delete;
        Writer& operator=(Writer&&) = delete;

        void set_vertex(const point_type& position, const vector_type& normal, const texcoord_type& texcoord);
        void set_object(const std::string& name);
        void set_group(const std::string& name);
        void set_face();
        void set_face_vertex(const point_type& position, const vector_type& normal, const texcoord_type& texcoord);
        void set_materials(const std::string& name);
        void set_material(const std::string& name);

        void clear();

    private:

        using id_type = long;

        void write_vertices();
        void close_section();
        void close_face();

        std::ofstream m_ofs;

        std::vector<point_type> m_positions;
        std::vector<vector_type> m_normals;
        std::vector<texcoord_type> m_texcoords;

        std::map<point_type, id_type, math::PriorityLess<point_type>> m_unique_positions;
        std::map<vector_type, id_type, math::PriorityLess<vector_type>> m_unique_normals;
        std::map<texcoord_type, id_type, math::PriorityLess<texcoord_type>> m_unique_texcoords;

        bool m_bVertices;
        bool m_bFace;
    };

} // namespace quetzal::wavefront_obj

//------------------------------------------------------------------------------
template<typename P, typename V, typename T>
quetzal::wavefront_obj::Writer<P, V, T>::Writer(const std::filesystem::path& filepath) :
    m_ofs(filepath, std::ios_base::binary),
    m_positions(),
    m_normals(),
    m_texcoords(),
    m_unique_positions(),
    m_unique_normals(),
    m_unique_texcoords(),
    m_bVertices(true),
    m_bFace(false)
{
    if (!m_ofs)
    {
        std::ostringstream oss;
        oss << "Error opening output file " << filepath;
        throw Exception(oss.str(), __FILE__, __LINE__);
    }
}

//------------------------------------------------------------------------------
template<typename P, typename V, typename T>
quetzal::wavefront_obj::Writer<P, V, T>::~Writer()
{
    close_face();
}

//------------------------------------------------------------------------------
template<typename P, typename V, typename T>
void quetzal::wavefront_obj::Writer<P, V, T>::set_vertex(const point_type& position, const vector_type& normal, const texcoord_type& texcoord)
{
    assert(m_bVertices);

    id_type idPosition = 0;
    const auto ip = m_unique_positions.find(position);
    if (ip != m_unique_positions.end())
    {
        idPosition = ip->second;
    }
    else
    {
        idPosition = static_cast<id_type>(m_positions.size());
        m_unique_positions.insert(std::make_pair(position, idPosition));
        m_positions.push_back(position);
    }

    id_type idNormal = 0;
    const auto in = m_unique_normals.find(normal);
    if (in != m_unique_normals.end())
    {
        idNormal = in->second;
    }
    else
    {
        idNormal = static_cast<id_type>(m_normals.size());
        m_unique_normals.insert(std::make_pair(normal, idNormal));
        m_normals.push_back(normal);
    }

    id_type idTexcoord = 0;
    const auto it = m_unique_texcoords.find(texcoord);
    if (it != m_unique_texcoords.end())
    {
        idTexcoord = it->second;
    }
    else
    {
        idTexcoord = static_cast<id_type>(m_texcoords.size());
        m_unique_texcoords.insert(std::make_pair(texcoord, idTexcoord));
        m_texcoords.push_back(texcoord);
    }

    return;
}

//------------------------------------------------------------------------------
template<typename P, typename V, typename T>
void quetzal::wavefront_obj::Writer<P, V, T>::set_object(const std::string& name)
{
    close_section();
    m_ofs << Keyword::Object << ' ' << name << '\n';
    return;
}

//------------------------------------------------------------------------------
template<typename P, typename V, typename T>
void quetzal::wavefront_obj::Writer<P, V, T>::set_group(const std::string& name)
{
    close_section();
    m_ofs << Keyword::Group << ' ' << (name != GroupNameDefault ? name : "") << '\n';
    return;
}

//------------------------------------------------------------------------------
template<typename P, typename V, typename T>
void quetzal::wavefront_obj::Writer<P, V, T>::set_face()
{
    close_section();
    m_ofs << Keyword::Face;
    m_bFace = true;
    return;
}

//------------------------------------------------------------------------------
template<typename P, typename V, typename T>
void quetzal::wavefront_obj::Writer<P, V, T>::set_face_vertex(const point_type& position, const vector_type& normal, const texcoord_type& texcoord)
{
    assert(m_bFace);

    const auto ip = m_unique_positions.find(position);
    assert(ip != m_unique_positions.end());
    m_ofs << ' ' << (ip->second + 1) << '/';

    if (!m_unique_texcoords.empty())
    {
        const auto it = m_unique_texcoords.find(texcoord);
        assert(it != m_unique_texcoords.end());
        m_ofs << (it->second + 1);
    }

    m_ofs << '/';

    if (!m_unique_normals.empty())
    {
        const auto in = m_unique_normals.find(normal);
        assert(in != m_unique_normals.end());
        m_ofs << (in->second + 1);
    }

    return;
}

//------------------------------------------------------------------------------
template<typename P, typename V, typename T>
void quetzal::wavefront_obj::Writer<P, V, T>::set_materials(const std::string& name)
{
    m_ofs << Keyword::Materials << ' ' << name << '\n';
    return;
}

//------------------------------------------------------------------------------
template<typename P, typename V, typename T>
void quetzal::wavefront_obj::Writer<P, V, T>::set_material(const std::string& name)
{
    m_ofs << Keyword::Material << ' ' << name << '\n';
    return;
}

//------------------------------------------------------------------------------
template<typename P, typename V, typename T>
void quetzal::wavefront_obj::Writer<P, V, T>::clear()
{
    m_positions.clear();
    m_normals.clear();
    m_texcoords.clear();
    m_unique_positions.clear();
    m_unique_normals.clear();
    m_unique_texcoords.clear();
    return;
}

//------------------------------------------------------------------------------
template<typename P, typename V, typename T>
void quetzal::wavefront_obj::Writer<P, V, T>::write_vertices()
{
    if (m_unique_normals.size() == 1 && m_normals[0].zero())
    {
        m_normals.clear();
        m_unique_normals.clear();
    }

    if (m_unique_texcoords.size() == 1)
    {
        m_texcoords.clear();
        m_unique_texcoords.clear();
    }

    m_ofs << "# Vertex Positions" << '\n';
    for (const point_type& position : m_positions)
    {
        m_ofs << Keyword::Position << ' ' << to_cartesian(position) << '\n';
    }

    if (!m_texcoords.empty())
    {
        m_ofs << '\n';
        m_ofs << "# Texture Coordinates" << '\n';
        for (const texcoord_type& texcoord : m_texcoords)
        {
            m_ofs << Keyword::Texcoord << ' ' << to_cartesian(texcoord) << '\n';
        }
    }

    if (!m_normals.empty())
    {
        m_ofs << '\n';
        m_ofs << "# Vertex Normals" << '\n';
        for (const vector_type& normal : m_normals)
        {
            m_ofs << Keyword::Normal << ' ' << to_cartesian(normal) << '\n';
        }
    }

    m_ofs << '\n';
    m_ofs << "# Faces" << '\n';

    return;
}

//------------------------------------------------------------------------------
template<typename P, typename V, typename T>
void quetzal::wavefront_obj::Writer<P, V, T>::close_section()
{
    if (m_bVertices)
    {
        write_vertices();
        m_bVertices = false;
    }
    else
    {
        close_face();
    }

    return;
}

//------------------------------------------------------------------------------
template<typename P, typename V, typename T>
void quetzal::wavefront_obj::Writer<P, V, T>::close_face()
{
    if (m_bFace)
    {
        m_ofs << '\n';
        m_bFace = false;
    }

    return;
}

#endif // QUETZAL_WAVEFRONT_OBJ_WRITER_HPP
