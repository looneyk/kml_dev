//------------------------------------------------------------------------------
// Frame.cpp
//------------------------------------------------------------------------------

#include "Frame.hpp"
#include "quetzal/xml/xml.hpp"
#include "quetzal/Exception.hpp"
#include "quetzal/string_util.hpp"
#include <fstream>
#include <sstream>

using namespace std;

namespace
{

    const string s_display = "display";
    const string s_frame = "frame";
    const string s_width = "width";
    const string s_height = "height";
    const string s_format = "format";

} // namespace

//------------------------------------------------------------------------------
quetzal::Frame::Frame(int w, int h, int n) :
    m_width(w),
    m_height(h),
    m_components(n),
    m_data(w * h * n)
{
}

//------------------------------------------------------------------------------
int quetzal::Frame::width() const
{
    return m_width;
}

//------------------------------------------------------------------------------
int quetzal::Frame::height() const
{
    return m_height;
}

//------------------------------------------------------------------------------
int quetzal::Frame::components() const
{
    return m_components;
}

//------------------------------------------------------------------------------
const vector<quetzal::Frame::component_type>& quetzal::Frame::data() const
{
    return m_data;
}

//------------------------------------------------------------------------------
vector<quetzal::Frame::component_type>& quetzal::Frame::data()
{
    return m_data;
}

//------------------------------------------------------------------------------
bool quetzal::Frame::load(const std::string& filename)
{
    xml::Document document;

    ifstream ifs(filename);
    if (!ifs.is_open())
    {
        ostringstream oss;
        oss << "Error opening xml file: " << filename << ": ";
        throw Exception(oss.str(), __FILE__, __LINE__);
    }

    ifs >> document;

    const xml::Element& emain = document.element();
    if (emain.name() != s_display)
    {
        ostringstream oss;
        oss << "Error parsing xml file " << filename << ": ";
        oss << "Not a valid frame file";
        throw Exception(oss.str(), __FILE__, __LINE__);
    }

    int w = 0;
    int h = 0;
    int n = 0;

    // Load elements
    for (auto& pe : emain.elements())
    {
        string name = pe->name();
        if (name == s_frame)
        {
            w = to_type<int>(pe->attribute_value(s_width));
            h = to_type<int>(pe->attribute_value(s_height));
            n = to_type<int>(pe->attribute_value(s_format));

            if (w == 0 || h == 0 || n == 0)
            {
                ostringstream oss;
                oss << "Error parsing frame xml file " << filename << ": ";
                oss << "Missing frame dimension: " << w << " " << h << " " << n;
                throw Exception(oss.str(), __FILE__, __LINE__);
            }

            set_size(w, h, n);

            istringstream iss(pe->value());
            for (size_t i = 0; i < m_data.size(); ++i)
            {
                iss >> m_data[i];
            }
        }
    }

    return true;
}

//------------------------------------------------------------------------------
bool quetzal::Frame::store(const std::string& filename) const
{
    xml::Document document;
    xml::Element& emain = document.element();
    emain.set_name(s_display);

    auto pe = emain.add_element(s_frame);
    pe->set_attribute(s_width, to_string(m_width));
    pe->set_attribute(s_height, to_string(m_height));
    pe->set_attribute(s_format, to_string(m_components));

    string indentation = string((emain.depth() + 2) * 4, ' ');
    int nRowComponents = m_width * m_components;
    ostringstream oss;
    for (int i = 0; i < m_height; ++i)
    {
        oss << endl << indentation;
        for (int j = 0; j < nRowComponents; ++j)
        {
            int n = (i * nRowComponents) + j * m_components;
            oss << m_data[n];
            if (j + 1 < nRowComponents)
            {
                if (m_components > 1 && ((j + 1) % m_components) == 0)
                {
                    oss << "  ";
                }
                else
                {
                    oss << " ";
                }
            }
        }
    }

    oss << endl << string((emain.depth() + 1) * 4, ' ');

    pe->set_value(oss.str());

    ofstream ofs(filename);
    if (ofs.fail())
    {
        return false;
    }

    ofs << document;
    return true;
}

//------------------------------------------------------------------------------
void quetzal::Frame::set_size(int w, int h, int components)
{
    m_width = w;
    m_height = h;
    m_components = components;
    m_data.resize(m_height * m_width * m_components);
    return;
}
