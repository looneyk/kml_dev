#if !defined(QUETZAL_DUNGEN_CELLTYPEPROPERTIES_HPP)
#define QUETZAL_DUNGEN_CELLTYPEPROPERTIES_HPP
//------------------------------------------------------------------------------
// dungen
// CellTypeProperties.hpp
//------------------------------------------------------------------------------

#include "CellType.hpp"
#include <iosfwd>
#include <string>
#include <unordered_map>

namespace quetzal::dungen
{

    //--------------------------------------------------------------------------
    class CellTypeProperties
    {
    public:

        CellTypeProperties() = default;
        CellTypeProperties(CellType type, const std::string& name, const std::string& symbol);
        CellTypeProperties(const CellTypeProperties&) = default;
        ~CellTypeProperties() = default;

        CellTypeProperties& operator=(const CellTypeProperties&) = default;

        CellType type() const;
        std::string name() const;
        std::string symbol() const;

        void print(std::ostream& os) const;

    private:

        CellType m_type;
        std::string m_name;
        std::string m_symbol;
    };

    std::ostream& operator<<(std::ostream& os, const CellTypeProperties& properties);

    extern std::unordered_map<CellType, CellTypeProperties> cell_types;

} // namespace quetzal::dungen

#endif // QUETZAL_DUNGEN_CELLTYPEPROPERTIES_HPP
