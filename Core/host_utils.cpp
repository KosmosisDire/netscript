#include "host_utils.h"

// Define NOMINMAX *before* including Windows.h to prevent macro conflicts
#define NOMINMAX
#include <Windows.h>
#include <shlwapi.h>
#include <filesystem>
#include <sstream>
#include <algorithm> // std::replace_if, std::min
#include <utility> // std::pair
#include <vector> // Added explicitly for std::vector
#include <string> // Added explicitly for std::string, std::stoi
#include <stdexcept> // For std::stoi exceptions
#include <iostream> // For potential debug output (optional)
#include <cstring> // For std::strlen

#pragma comment(lib, "shlwapi.lib") // Needed for PathRemoveFileSpecA

namespace Core::HostUtils
{
    // Helper function to parse version strings like "9.0.1"
    std::vector<int> parse_version(const std::string& versionStr)
    {
        std::vector<int> versionParts;
        std::stringstream ss(versionStr);
        std::string segment;

        while (std::getline(ss, segment, '.'))
        {
            try {
                // Skip empty segments which can happen with trailing dots etc.
                if (!segment.empty()) {
                    // Check for non-digit characters (like -preview) and stop there
                    size_t nonDigitPos = segment.find_first_not_of("0123456789");
                    if (nonDigitPos != std::string::npos) {
                        // Only parse the digit part before the non-digit part
                        if (nonDigitPos > 0) {
                           versionParts.push_back(std::stoi(segment.substr(0, nonDigitPos)));
                        }
                        // Stop parsing further segments for this version string
                        break;
                    } else {
                        versionParts.push_back(std::stoi(segment));
                    }
                }
            } catch (const std::invalid_argument& e) {
                 // Handle segments that are not numbers at all (e.g., "alpha")
                 (void)e; // Suppress unused warning
                 // Stop parsing for this version string
                 break; // Or return {} depending on desired strictness
            } catch (const std::out_of_range& e) {
                 // Handle numbers too large for int
                 (void)e; // Suppress unused warning
                 // Stop parsing for this version string
                 break; // Or return {}
            }
        }
        return versionParts;
    }

    // Helper function to compare version vectors
    bool is_version_greater(const std::vector<int>& v1, const std::vector<int>& v2)
    {
        // Handle empty vectors comparison gracefully
        if (v1.empty() && v2.empty()) return false;
        if (v1.empty()) return false; // v2 is greater or equal
        if (v2.empty()) return true;  // v1 is greater

        size_t len = std::min(v1.size(), v2.size()); // Use std::min
        for (size_t i = 0; i < len; ++i) {
            if (v1[i] > v2[i]) return true;
            if (v1[i] < v2[i]) return false;
        }
        // If prefixes match, the longer version is considered greater (e.g., 9.0.1 > 9.0)
        return v1.size() > v2.size();
    }


    std::string find_latest_dot_net_runtime(int majorVersion)
    {
        const std::filesystem::path baseRuntimePath =
            "C:/Program Files/dotnet/shared/Microsoft.NETCore.App";

        if (!std::filesystem::exists(baseRuntimePath))
            return ""; // .NET runtime base directory not found

        std::pair<std::vector<int>, std::filesystem::path> latestVersion = { {}, {} };

        try
        {
            for (const auto& dirEntry : std::filesystem::directory_iterator(baseRuntimePath))
            {
                if (!dirEntry.is_directory())
                    continue;

                const auto& dirPath = dirEntry.path();
                const auto& dirName = dirPath.filename().string();
                if (dirName.empty())
                    continue;

                std::vector<int> currentVersionParts = parse_version(dirName);

                // Check if parsing was successful and if it meets the minimum major version
                if (!currentVersionParts.empty() && currentVersionParts[0] >= majorVersion)
                {
                    if (latestVersion.second.empty() || is_version_greater(currentVersionParts, latestVersion.first))
                    {
                         latestVersion = { currentVersionParts, dirPath };
                    }
                }
            }
        }
        catch (const std::filesystem::filesystem_error& e) {
            // Handle potential errors during directory iteration (e.g., permissions)
            std::cerr << "Filesystem error while searching for runtime: " << e.what() << std::endl;
            return "";
        }
        catch (const std::exception& e) {
            // Catch other potential standard exceptions during parsing/comparison
            std::cerr << "Standard exception while searching for runtime: " << e.what() << std::endl;
            return "";
        }


        if (!latestVersion.second.empty())
        {
            auto dotnetPath = latestVersion.second.string();
            // Ensure backslashes for CoreCLR initialization path properties
            std::replace_if(dotnetPath.begin(), dotnetPath.end(),
                [](char c){ return c == '/'; }, '\\');
            return dotnetPath;
        }

        return ""; // No suitable version found
    }

    std::string build_tpa_list(const std::string& directory)
    {
        // Check if directory is empty or doesn't exist to avoid searching invalid paths
        if (directory.empty() || !std::filesystem::exists(directory) || !std::filesystem::is_directory(directory)) {
            return "";
        }

        const std::string searchPath = directory + "\\*.dll";
        static constexpr char PATH_DELIMITER = ';';
        std::ostringstream tpaListStream;

        WIN32_FIND_DATAA findData;
        HANDLE fileHandle = FindFirstFileA(searchPath.c_str(), &findData);

        if (fileHandle != INVALID_HANDLE_VALUE)
        {
            do
            {
                // Append the full path of the assembly to the list
                tpaListStream << directory << '\\' << findData.cFileName << PATH_DELIMITER;
            }
            while (FindNextFileA(fileHandle, &findData));
            FindClose(fileHandle);
        }
        else {
            // Optionally report if FindFirstFileA fails (e.g., permissions, path issues)
            // DWORD error = GetLastError();
            // if (error != ERROR_FILE_NOT_FOUND) { // Ignore "no files found" error
            //     std::cerr << "Warning: Could not search for DLLs in '" << directory << "'. Error code: " << error << std::endl;
            // }
        }


        return tpaListStream.str();
    }

    std::string get_current_executable_directory()
    {
        std::string path(MAX_PATH, '\0');
        // Use MAX_PATH as initial buffer size, but check if it was enough
        DWORD pathLen = GetModuleFileNameA(nullptr, path.data(), MAX_PATH);

        if (pathLen == 0) {
            std::cerr << "Error: GetModuleFileNameA failed. Error code: " << GetLastError() << std::endl;
            return "";
        }
        else if (pathLen == MAX_PATH) {
             // Buffer might have been too small. Can attempt to resize and retry,
             // but for simplicity, we'll just report a warning or error here.
             std::cerr << "Warning: Path returned by GetModuleFileNameA might be truncated." << std::endl;
             // Still attempt to remove file spec from potentially truncated path
        }


        // Resize to actual length returned by GetModuleFileNameA BEFORE PathRemoveFileSpecA
        path.resize(pathLen);

        // PathRemoveFileSpecA modifies in place. Check return value.
        // Note: It expects a non-const char*, hence path.data()
        if (PathRemoveFileSpecA(path.data())) {
             // Resize again to the new length after removing the file name
             path.resize(std::strlen(path.data()));
             return path;
        }
        else {
            // PathRemoveFileSpecA failed. Maybe it was already a root path?
            // Or maybe GetModuleFileNameA returned a path without a filename?
            // Return the path obtained from GetModuleFileNameA as a fallback if it seems valid.
             if (!path.empty() && std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
                 return path; // Return the directory path if it seems valid
             }
             std::cerr << "Error: PathRemoveFileSpecA failed on path: " << path << std::endl;
             return "";
        }
    }

} // namespace Core::HostUtils