#if !defined(QUETZAL_SYSTEM_PATH_HPP)
#define QUETZAL_SYSTEM_PATH_HPP
//------------------------------------------------------------------------------
// system_path.hpp
//------------------------------------------------------------------------------

#include <filesystem>
#include <shlobj.h>

namespace quetzal
{

    std::filesystem::path system_path(REFKNOWNFOLDERID rfid);

} // namespace quetzal

#endif // QUETZAL_SYSTEM_PATH_HPP
