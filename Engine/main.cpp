#include <iostream>
#include <string>
#include <vector>
#include <cstdlib> // EXIT_SUCCESS, EXIT_FAILURE
#include <thread>  // For std::this_thread::sleep_for
#include <chrono>  // For std::chrono::seconds

// Include Core library headers
#include "dot_net_runtime.h" // Correct include path
#include "host_utils.h"      // Correct include path

// Define the function pointer type for the managed delegate
using PingDelegate = void(*)();

int main()
{
    std::cout << "Engine starting..." << std::endl;

    // --- Find .NET Runtime ---
    std::cout << "Searching for .NET 9+ runtime..." << std::endl;
    const int requiredMajorVersion = 9;
    std::string runtimePath = Core::HostUtils::find_latest_dot_net_runtime(requiredMajorVersion);

    if (runtimePath.empty())
    {
        std::cerr << "Error: Could not find a compatible .NET "
                  << requiredMajorVersion << "+ runtime." << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << "Found .NET Runtime at: " << runtimePath << std::endl;

    // --- Get Application Base Directory ---
    std::string appBasePath = Core::HostUtils::get_current_executable_directory();
    if (appBasePath.empty())
    {
        std::cerr << "Error: Could not determine the application base directory." << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << "Application Base Path: " << appBasePath << std::endl;

    // --- Build Trusted Platform Assemblies (TPA) List ---
    std::cout << "Building TPA list..." << std::endl;
    // IMPORTANT: Add ScriptAPI.dll to the TPA list!
    std::string tpaList = Core::HostUtils::build_tpa_list(runtimePath); // Runtime DLLs
    tpaList += Core::HostUtils::build_tpa_list(appBasePath);            // Add DLLs in executable directory (Core.dll, Engine.exe, ScriptAPI.dll)
    if (tpaList.empty())
    {
       std::cerr << "Warning: TPA list is empty. CoreCLR might fail to load assemblies." << std::endl;
    } else {
         std::cout << "TPA list built." << std::endl;
    }


    // --- Initialize CoreCLR ---
    std::cout << "Initializing CoreCLR..." << std::endl;
    Core::DotNetRuntime runtime;

    bool initialized = runtime.initialize(runtimePath, appBasePath, tpaList);

    if (!initialized)
    {
        std::cerr << "Failed to initialize the .NET runtime." << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "CoreCLR Initialized successfully!" << std::endl;

    // --- Get Delegate for ScriptAPI::Ping ---
    std::cout << "Getting delegate for ScriptAPI.EngineInterface.Ping..." << std::endl;
    PingDelegate pingFunc = nullptr;
    bool delegateCreated = runtime.create_delegate(
        "ScriptAPI",                 // Assembly name (without .dll)
        "ScriptAPI.EngineInterface", // Fully qualified type name
        "Ping",                      // Static method name
        &pingFunc                    // Address of the function pointer
    );

    if (!delegateCreated || pingFunc == nullptr)
    {
        std::cerr << "Failed to create delegate for ScriptAPI.EngineInterface.Ping." << std::endl;
        // Decide if this is fatal. For now, continue and let shutdown handle cleanup.
    }
    else
    {
        std::cout << "Delegate created successfully. Calling Ping..." << std::endl;
        // --- Call the Managed Method ---
        try
        {
             pingFunc();
        }
        catch (const std::exception& e) // Catch potential exceptions crossing the boundary
        {
            std::cerr << "!!! Native Exception caught calling Ping delegate: " << e.what() << std::endl;
        }
        catch (...) // Catch any other non-standard exceptions
        {
             std::cerr << "!!! Unknown exception caught calling Ping delegate." << std::endl;
        }
    }


    // --- Runtime Usage Placeholder ---
    std::cout << "Simulating engine work for 2 seconds..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // --- Shutdown CoreCLR ---
    std::cout << "Shutting down CoreCLR..." << std::endl;
    bool shutdownSuccess = runtime.shutdown();

    if (!shutdownSuccess)
    {
        std::cerr << "CoreCLR shutdown reported an error." << std::endl;
    } else {
        std::cout << "CoreCLR shutdown successful." << std::endl;
    }

    std::cout << "Engine exiting." << std::endl;
    return EXIT_SUCCESS;
}