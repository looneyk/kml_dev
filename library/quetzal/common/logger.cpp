//------------------------------------------------------------------------------
// logger.cpp
//------------------------------------------------------------------------------

#include "logger.hpp"

using namespace std;

quetzal::Log<ofstream> quetzal::log(quetzal::LogLevel::All, log_path / "quetzal.log");
