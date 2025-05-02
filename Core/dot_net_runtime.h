#pragma once

#include "import_export.h" // For DLL_API
#include <string>
#include <vector>
#include <stdexcept> // For runtime_error
#include <sstream>   // For ostringstream
#include <iomanip>   // For setfill, setw, hex
#include <Windows.h> // HMODULE

// Forward declare coreclr types to avoid including coreclrhost.h in the public header
struct coreclr_initialize_ptr_stub;
struct coreclr_shutdown_ptr_stub;
struct coreclr_create_delegate_ptr_stub;
struct coreclr_execute_assembly_ptr_stub; // Added for completeness, though not used yet

namespace Core
{
    class DLL_API DotNetRuntime
    {
    public:
        DotNetRuntime();
        ~DotNetRuntime();

        // Non-copyable
        DotNetRuntime(const DotNetRuntime&) = delete;
        DotNetRuntime& operator=(const DotNetRuntime&) = delete;

        // Initializes the CoreCLR runtime.
        // runtimeDirectory: Path to the specific .NET runtime version (e.g., C:\...\9.0.0).
        // appDomainBaseDirectory: Usually the directory containing the host executable.
        // tpaList: Semicolon-delimited list of trusted platform assembly paths.
        bool initialize(
            const std::string& runtimeDirectory,
            const std::string& appDomainBaseDirectory,
            const std::string& tpaList);

        // Shuts down the CoreCLR runtime.
        bool shutdown();

        // Creates a function pointer to a static managed method.
        // assemblyName: Name of the assembly (without .dll extension).
        // typeName: Fully qualified type name (Namespace.ClassName).
        // methodName: Name of the static method.
        // delegate: Output pointer where the native function pointer will be stored.
        template <typename TDelegate>
        bool create_delegate(
            const std::string& assemblyName,
            const std::string& typeName,
            const std::string& methodName,
            TDelegate* delegatePtr)
        {
            if (!is_initialized() || !createManagedDelegate_)
            {
                return false;
            }

            int result = createManagedDelegate_(
                hostHandle_,
                domainId_,
                assemblyName.c_str(),
                typeName.c_str(),
                methodName.c_str(),
                reinterpret_cast<void**>(delegatePtr)
            );

            if (result < 0)
            {
                // Consider logging the error instead of throwing from a template header?
                // Or return specific error codes. For now, just return false.
                // std::ostringstream oss;
                // oss << std::hex << std::setfill('0') << std::setw(8)
                //     << "Failed to create delegate for " << typeName << "." << methodName
                //     << ". Error 0x" << result;
                // throw std::runtime_error(oss.str());
                *delegatePtr = nullptr; // Ensure delegate is null on failure
                return false;
            }
            return true;
        }

        // Checks if the runtime is currently initialized.
        bool is_initialized() const;

    private:
        // Typedefs for CoreCLR function pointers using opaque structs
        using coreclr_initialize_ptr      = int (*)(const char*, const char*, int, const char**, const char**, void**, unsigned int*);
        using coreclr_shutdown_ptr        = int (*)(void*, unsigned int);
        using coreclr_create_delegate_ptr = int (*)(void*, unsigned int, const char*, const char*, const char*, void**);
        //using coreclr_execute_assembly_ptr= int (*)(void*, unsigned int, int, const char**, const char*, unsigned int*); // If needed later

        HMODULE coreClrLib_ = nullptr;
        void* hostHandle_ = nullptr;
        unsigned int domainId_ = 0;

        // Function Pointers
        coreclr_initialize_ptr      initializeCoreClr_ = nullptr;
        coreclr_shutdown_ptr        shutdownCoreClr_ = nullptr;
        coreclr_create_delegate_ptr createManagedDelegate_ = nullptr;
        // coreclr_execute_assembly_ptr executeAssembly_ = nullptr; // If needed later

        // Helper to get function pointers from coreclr.dll
        template<typename TFunc>
        bool get_core_clr_function(const std::string& functionName, TFunc* funcPtr);
    };

} // namespace Core