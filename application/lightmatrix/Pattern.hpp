#if !defined(QUETZAL_LIGHTMATRIX_PATTERN_HPP)
#define QUETZAL_LIGHTMATRIX_PATTERN_HPP
//------------------------------------------------------------------------------
// Light Matrix Simulation
// Pattern.hpp
//------------------------------------------------------------------------------

#include "Color.hpp"
#include <array>
#include <filesystem>
#include <optional>
#include <tuple>
#include <vector>

namespace quetzal::lightmatrix
{

    //----------------------------------------------------------------------
    class Pattern
    {
    public:

        class Panel;
        class Animation;

        using size_type = std::array<size_t, 2>;
        using colors_type = std::vector<Color>;

        Pattern(size_type size = {});
        Pattern(const Pattern&) = default;
        Pattern(Pattern&&) = default;
        ~Pattern() = default;

        Pattern& operator=(const Pattern&) = default;
        Pattern& operator=(Pattern&&) = default;

        size_type size() const;
        void set_size(size_type size);

        const Color& color(size_t x, size_t y) const;
        Color& color(size_t x, size_t y);

        const colors_type& colors() const;
        colors_type& colors();

        void load(const std::filesystem::path& file_path);
        void store(const std::filesystem::path& file_path);

    private:

        size_type m_size;
        colors_type m_colors;
    };

    Pattern lerp(const Pattern& pattern0, const Pattern& pattern1, float t);

    Pattern load_pattern(const std::filesystem::path& file_path);

    // Load the patter and its associated preferred panel and animation settings
    std::tuple<Pattern, Panel, Animation> load_pattern_layout(const std::filesystem::path& file_path);
    void store_pattern_layout(const std::filesystem::path& file_path, const Pattern& pattern, const Panel& panel, const Animation& animation);

} // namespace quetzal::lightmatrix

#endif // QUETZAL_LIGHTMATRIX_PATTERN_HPP
