#include "dot_net_runtime.h"
#include "host_utils.h"       // Include for get_current_executable_directory, etc.

#include <coreclrhost.h>      // The actual CoreCLR header - Ensure this path is found via CMake includes
#include <filesystem>         // For std::filesystem::current_path
#include <array>
#include <iostream>           // For basic error output
#include <Windows.h>          // Include Windows.h here where needed

namespace Core
{
    DotNetRuntime::DotNetRuntime() = default;

    DotNetRuntime::~DotNetRuntime()
    {
        // Ensure shutdown is called even if the user forgets
        if (is_initialized())
        {
            shutdown();
        }
        if (coreClrLib_)
        {
            FreeLibrary(coreClrLib_);
        }
    }

    template<typename TFunc>
    bool DotNetRuntime::get_core_clr_function(const std::string& functionName, TFunc* funcPtr)
    {
        if (!coreClrLib_) return false;

        *funcPtr = reinterpret_cast<TFunc>(GetProcAddress(coreClrLib_, functionName.c_str()));
        if (!*funcPtr)
        {
            std::cerr << "Error: Failed to get CoreCLR function pointer: " << functionName << std::endl;
            return false;
        }
        return true;
    }


    bool DotNetRuntime::initialize(
        const std::string& runtimeDirectory,
        const std::string& appDomainBaseDirectory,
        const std::string& tpaList)
    {
        if (is_initialized())
        {
            std::cerr << "Warning: DotNetRuntime already initialized." << std::endl;
            return true; // Already initialized
        }

        // Construct the absolute path to coreclr.dll
        std::filesystem::path coreClrPath = runtimeDirectory;
        coreClrPath /= "coreclr.dll";

        // Load the CoreCLR library
        coreClrLib_ = LoadLibraryExA(coreClrPath.string().c_str(), nullptr, 0);
        if (!coreClrLib_)
        {
            std::cerr << "Error: Failed to load CoreCLR from " << coreClrPath << ". Error code: " << GetLastError() << std::endl;
            return false;
        }

        // Get required CoreCLR function pointers
        if (!get_core_clr_function("coreclr_initialize", &initializeCoreClr_) ||
            !get_core_clr_function("coreclr_shutdown", &shutdownCoreClr_) ||
            !get_core_clr_function("coreclr_create_delegate", &createManagedDelegate_))
            // Add other functions like coreclr_execute_assembly if needed
        {
            FreeLibrary(coreClrLib_);
            coreClrLib_ = nullptr;
            return false;
        }

        // Define CoreCLR properties
        // TRUSTED_PLATFORM_ASSEMBLIES: Assemblies CoreCLR will trust implicitly. Includes runtime assemblies and our app/API assemblies.
        // APP_PATHS: Directory where the host executable is, used for probing dependencies.
        std::array propertyKeys = {
            "TRUSTED_PLATFORM_ASSEMBLIES",
            "APP_PATHS"
            // Optional: NATIVE_DLL_SEARCH_DIRECTORIES: Directories to probe for native dependencies of managed code
            // Optional: PLATFORM_RESOURCE_ROOTS: Directories where satellite resource assemblies might be found
            // Optional: AppDomainCompatSwitch: e.g., "UseLatestBehaviorWhenNotSpecified"
        };

        std::array propertyValues = {
            tpaList.c_str(),
            appDomainBaseDirectory.c_str()
        };

        // Initialize CoreCLR
        int result = initializeCoreClr_(
            appDomainBaseDirectory.c_str(), // App domain base path
            "MyEngineHostAppDomain",        // Friendly name for the app domain
            static_cast<int>(propertyKeys.size()),
            propertyKeys.data(),
            propertyValues.data(),
            &hostHandle_,                   // Output: host handle
            &domainId_                      // Output: domain ID
        );

        if (result < 0)
        {
            std::ostringstream oss;
            oss << std::hex << std::setfill('0') << std::setw(8)
                << "Error: Failed to initialize CoreCLR. HRESULT: 0x" << result;
            std::cerr << oss.str() << std::endl;
            FreeLibrary(coreClrLib_);
            coreClrLib_ = nullptr;
            hostHandle_ = nullptr; // Ensure handle is null on failure
            return false;
        }

        // Set the C++ current working directory to the app base directory
        // This helps ensure relative paths work as expected for both native and managed code
        try {
             std::filesystem::current_path(appDomainBaseDirectory);
        } catch(const std::filesystem::filesystem_error& e) {
             std::cerr << "Warning: Could not set current working directory to "
                       << appDomainBaseDirectory << ". Error: " << e.what() << std::endl;
             // Continue initialization even if this fails, but paths might be less predictable.
        }


        return true; // Initialization successful
    }

    bool DotNetRuntime::shutdown()
    {
        if (!is_initialized() || !shutdownCoreClr_)
        {
            return false; // Not initialized or shutdown function not found
        }

        int result = shutdownCoreClr_(hostHandle_, domainId_);

        // Reset state regardless of shutdown result, as the host is likely unusable
        hostHandle_ = nullptr;
        domainId_ = 0;
        initializeCoreClr_ = nullptr;
        shutdownCoreClr_ = nullptr;
        createManagedDelegate_ = nullptr;

        if (coreClrLib_)
        {
            FreeLibrary(coreClrLib_);
            coreClrLib_ = nullptr;
        }

        if (result < 0)
        {
            std::ostringstream oss;
            oss << std::hex << std::setfill('0') << std::setw(8)
                << "Error: Failed to shutdown CoreCLR. HRESULT: 0x" << result;
            std::cerr << oss.str() << std::endl;
            return false;
        }

        return true;
    }

    bool DotNetRuntime::is_initialized() const
    {
        // Consider hostHandle_ the primary indicator of successful initialization
        return hostHandle_ != nullptr;
    }

} // namespace Core