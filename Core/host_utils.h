#pragma once

#include "import_export.h" // Include DLL_API definition
#include <string>
#include <vector>

namespace Core::HostUtils
{
    // Finds the path to the highest installed .NET 9+ runtime.
    // Returns an empty string if no suitable runtime is found.
    DLL_API std::string find_latest_dot_net_runtime(int majorVersion = 9);

    // Builds a semicolon-delimited list of DLLs in a given directory.
    DLL_API std::string build_tpa_list(const std::string& directory);

    // Gets the directory containing the current executable/module.
    DLL_API std::string get_current_executable_directory();

} // namespace Core::HostUtils