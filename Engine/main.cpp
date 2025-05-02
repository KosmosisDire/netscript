#include <iostream>
#include <string>
#include <vector>
#include <cstdlib> // EXIT_SUCCESS, EXIT_FAILURE
#include <thread>  // For std::this_thread::sleep_for
#include <chrono>  // For std::chrono::seconds

// Include Core library headers
#include "dot_net_runtime.h" // Correct include path
#include "host_utils.h"      // Correct include path

// Define the function pointer types for the managed delegates
using InitDelegate = bool(*)();       // Corresponds to ScriptAPI::EngineInterface::Init
using SayHelloDelegate = void(*)();   // Corresponds to ScriptAPI::EngineInterface::CallSayHello
using PingDelegate = void(*)();       // Corresponds to ScriptAPI::EngineInterface::Ping (optional)


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
    std::string tpaList = Core::HostUtils::build_tpa_list(runtimePath); // Runtime DLLs
    tpaList += Core::HostUtils::build_tpa_list(appBasePath);            // Core.dll, ScriptAPI.dll, ManagedScripts.dll, etc.
    if (tpaList.empty())
    {
       std::cerr << "Warning: TPA list is empty." << std::endl;
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

    // --- Get Delegates for ScriptAPI ---
    std::cout << "Getting delegates from ScriptAPI..." << std::endl;

    InitDelegate scriptApiInit = nullptr;
    SayHelloDelegate scriptApiSayHello = nullptr;
    // PingDelegate scriptApiPing = nullptr; // Optional

    bool initDelegateOk = runtime.create_delegate("ScriptAPI", "ScriptAPI.EngineInterface", "Init", &scriptApiInit);
    bool sayHelloDelegateOk = runtime.create_delegate("ScriptAPI", "ScriptAPI.EngineInterface", "CallSayHello", &scriptApiSayHello);
    // bool pingDelegateOk = runtime.create_delegate("ScriptAPI", "ScriptAPI.EngineInterface", "Ping", &scriptApiPing); // Optional

    if (!initDelegateOk || !sayHelloDelegateOk /* || !pingDelegateOk */ )
    {
         std::cerr << "Failed to get one or more required delegates from ScriptAPI." << std::endl;
         runtime.shutdown(); // Shutdown runtime before exiting
         return EXIT_FAILURE;
    }
     std::cout << "Delegates obtained successfully." << std::endl;


    // --- Initialize ScriptAPI Environment ---
    std::cout << "Calling ScriptAPI Init..." << std::endl;
    bool scriptApiInitialized = false;
    try
    {
        scriptApiInitialized = scriptApiInit();
    }
    catch (...) { // Catch potential exceptions crossing the boundary
        std::cerr << "!!! Unknown exception caught calling ScriptAPI Init delegate." << std::endl;
    }

    if (!scriptApiInitialized) {
        std::cerr << "ScriptAPI initialization failed." << std::endl;
        runtime.shutdown();
        return EXIT_FAILURE;
    }
    std::cout << "ScriptAPI Init completed." << std::endl;


    // --- Call the C# Method via ScriptAPI ---
    std::cout << "Calling ScriptAPI CallSayHello..." << std::endl;
    try
    {
        scriptApiSayHello();
    }
    catch (...) {
        std::cerr << "!!! Unknown exception caught calling ScriptAPI CallSayHello delegate." << std::endl;
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