#pragma once

// Select the correct export system based on the compiler and OS
#if defined(_WIN32) || defined(__CYGWIN__)
    #if defined(_MSC_VER)
        #define API_EXPORT __declspec(dllexport)
        #define API_IMPORT __declspec(dllimport)
    #else // MinGW or other Windows compilers
        #define API_EXPORT __attribute__((dllexport))
        #define API_IMPORT __attribute__((dllimport))
    #endif
#elif defined(__GNUC__) && __GNUC__ >= 4 // Linux, macOS
    #define API_EXPORT __attribute__((visibility("default")))
    #define API_IMPORT __attribute__((visibility("default")))
#else // Unsupported compiler/platform
    #define API_EXPORT
    #define API_IMPORT
    #pragma warning "Unsupported compiler/platform for DLL export/import."
#endif

// Define DLL_API based on whether we are building the DLL or using it
#ifdef DLL_API_EXPORT // Defined by the DLL project (Core) during build
    #define DLL_API API_EXPORT
#else
    #define DLL_API API_IMPORT // Used by projects linking against the DLL (Engine)
#endif