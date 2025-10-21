#if !defined(QUETZAL_FRAME_HPP)
#define QUETZAL_FRAME_HPP
//------------------------------------------------------------------------------
// Frame.hpp
//------------------------------------------------------------------------------

#include <string>
#include <vector>

namespace quetzal
{

    //--------------------------------------------------------------------------
    class Frame
    {
    public:

        typedef float component_type;

        Frame(int w, int h, int n);
        Frame() = default;
        Frame(const Frame&) = default;
        ~Frame() = default;

        Frame& operator=(const Frame&) = default;

        int width() const;
        int height() const;
        int components() const;
        const std::vector<component_type>& data() const;
        std::vector<component_type>& data();

        bool load(const std::string& filename);
        bool store(const std::string& filename) const;

    private:

        void set_size(int w, int h, int components);

        int m_width;
        int m_height;
        int m_components;
        std::vector<component_type> m_data;
    };

} // namespace quetzal

#endif // QUETZAL_FRAME_HPP
