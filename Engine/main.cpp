#include <iostream>
#include <string>
#include <vector>
#include <cstdlib> // EXIT_SUCCESS, EXIT_FAILURE
#include <thread>  // For std::this_thread::sleep_for
#include <chrono>  // For std::chrono::seconds
#include <Windows.h> // For GetAsyncKeyState, VK_ESCAPE

// Include Core library headers
#include "dot_net_runtime.h" // Correct include path
#include "host_utils.h"      // Correct include path

// Define the function pointer types for the managed delegates
using InitDelegate = bool(*)();
using ShutdownDelegate = void(*)();
using AddScriptDelegate = bool(*)(int, const char*); // Pass script name as const char* from C++
using ExecuteStartDelegate = void(*)(int);
using ExecuteUpdateDelegate = void(*)();

// Helper to convert std::string to const char* suitable for delegate call
// NOTE: Be careful with lifetime if the string is temporary.
// For simple literals or strings with lifetime managed elsewhere, this is okay.
const char* StringToChar(const std::string& str) {
    return str.c_str();
}


int main()
{
    std::cout << "Engine starting..." << std::endl;

    // --- Find .NET Runtime ---
    std::cout << "Searching for .NET 9+ runtime..." << std::endl;
    const int requiredMajorVersion = 9;
    std::string runtimePath = Core::HostUtils::find_latest_dot_net_runtime(requiredMajorVersion);
    if (runtimePath.empty()) {
        std::cerr << "Error: Could not find a compatible .NET " << requiredMajorVersion << "+ runtime." << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << "Found .NET Runtime at: " << runtimePath << std::endl;

    // --- Get Application Base Directory ---
    std::string appBasePath = Core::HostUtils::get_current_executable_directory();
    if (appBasePath.empty()) {
        std::cerr << "Error: Could not determine the application base directory." << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << "Application Base Path: " << appBasePath << std::endl;

    // --- Build Trusted Platform Assemblies (TPA) List ---
    std::cout << "Building TPA list..." << std::endl;
    std::string tpaList = Core::HostUtils::build_tpa_list(runtimePath);
    tpaList += Core::HostUtils::build_tpa_list(appBasePath);
    if (tpaList.empty()) {
       std::cerr << "Warning: TPA list is empty." << std::endl;
    } else {
         std::cout << "TPA list built." << std::endl;
    }

    // --- Initialize CoreCLR ---
    std::cout << "Initializing CoreCLR..." << std::endl;
    Core::DotNetRuntime runtime;
    bool initialized = runtime.initialize(runtimePath, appBasePath, tpaList);
    if (!initialized) {
        std::cerr << "Failed to initialize the .NET runtime." << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << "CoreCLR Initialized successfully!" << std::endl;

    // --- Get Delegates for ScriptAPI ---
    std::cout << "Getting delegates from ScriptAPI..." << std::endl;
    InitDelegate scriptApiInit = nullptr;
    ShutdownDelegate scriptApiShutdown = nullptr;
    AddScriptDelegate scriptApiAddScript = nullptr;
    ExecuteStartDelegate scriptApiExecuteStart = nullptr;
    ExecuteUpdateDelegate scriptApiExecuteUpdate = nullptr;

    bool delegatesOk = true;
    delegatesOk &= runtime.create_delegate("ScriptAPI", "ScriptAPI.EngineInterface", "Init", &scriptApiInit);
    delegatesOk &= runtime.create_delegate("ScriptAPI", "ScriptAPI.EngineInterface", "Shutdown", &scriptApiShutdown);
    delegatesOk &= runtime.create_delegate("ScriptAPI", "ScriptAPI.EngineInterface", "AddScript", &scriptApiAddScript);
    delegatesOk &= runtime.create_delegate("ScriptAPI", "ScriptAPI.EngineInterface", "ExecuteStartForEntity", &scriptApiExecuteStart);
    delegatesOk &= runtime.create_delegate("ScriptAPI", "ScriptAPI.EngineInterface", "ExecuteUpdate", &scriptApiExecuteUpdate);

    if (!delegatesOk || !scriptApiInit || !scriptApiShutdown || !scriptApiAddScript || !scriptApiExecuteStart || !scriptApiExecuteUpdate) {
         std::cerr << "Failed to get one or more required delegates from ScriptAPI." << std::endl;
         runtime.shutdown();
         return EXIT_FAILURE;
    }
     std::cout << "Delegates obtained successfully." << std::endl;

    // --- Initialize ScriptAPI Environment ---
    std::cout << "Calling ScriptAPI Init..." << std::endl;
    bool scriptApiInitialized = false;
    try { scriptApiInitialized = scriptApiInit(); }
    catch (...) { std::cerr << "!!! Exception caught calling ScriptAPI Init delegate." << std::endl; }

    if (!scriptApiInitialized) {
        std::cerr << "ScriptAPI initialization failed." << std::endl;
        runtime.shutdown();
        return EXIT_FAILURE;
    }
    std::cout << "ScriptAPI Init completed." << std::endl;

    // --- Add a Script Instance ---
    int playerEntityId = 0; // Example entity ID
    std::string scriptName = "MyFirstScript"; // Name must match class name found by DiscoverScriptTypes
    std::cout << "Adding script '" << scriptName << "' to entity " << playerEntityId << "..." << std::endl;
    bool scriptAdded = false;
    try { scriptAdded = scriptApiAddScript(playerEntityId, scriptName.c_str()); }
    catch (...) { std::cerr << "!!! Exception caught calling ScriptAPI AddScript delegate." << std::endl; }

    if (!scriptAdded) {
         std::cerr << "Failed to add script " << scriptName << std::endl;
         // Decide if this is fatal, for now continue but Update won't run
    } else {
        std::cout << "Script added." << std::endl;

        // --- Execute Start for the entity ---
        std::cout << "Executing Start() for entity " << playerEntityId << "..." << std::endl;
         try { scriptApiExecuteStart(playerEntityId); }
         catch (...) { std::cerr << "!!! Exception caught calling ScriptAPI ExecuteStart delegate." << std::endl; }
    }

    // --- Main Engine Loop Simulation ---
    std::cout << "Starting main loop (Press ESC to exit)..." << std::endl;
    bool running = true;
    int frameCount = 0;
    while(running)
    {
        // Check for exit condition (e.g., pressing ESC key)
        // Use GetAsyncKeyState for non-blocking check
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            running = false;
            std::cout << "\nESC pressed, exiting loop." << std::endl;
            continue; // Skip update on exit frame
        }

        // --- Execute Script Updates ---
        try { scriptApiExecuteUpdate(); }
        catch (...) { std::cerr << "!!! Exception caught calling ScriptAPI ExecuteUpdate delegate." << std::endl; }

        // Simulate frame delay
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
        frameCount++;

        // Optional: Limit loop duration for testing
        // if (frameCount > 300) running = false; // Exit after ~5 seconds
    }
    std::cout << "Exited main loop after " << frameCount << " frames." << std::endl;

    // --- Shutdown ScriptAPI ---
    std::cout << "Calling ScriptAPI Shutdown..." << std::endl;
    try { scriptApiShutdown(); }
    catch (...) { std::cerr << "!!! Exception caught calling ScriptAPI Shutdown delegate." << std::endl; }

    // --- Shutdown CoreCLR ---
    std::cout << "Shutting down CoreCLR..." << std::endl;
    bool shutdownSuccess = runtime.shutdown();
    if (!shutdownSuccess) {
        std::cerr << "CoreCLR shutdown reported an error." << std::endl;
    } else {
        std::cout << "CoreCLR shutdown successful." << std::endl;
    }

    std::cout << "Engine exiting." << std::endl;
    return EXIT_SUCCESS;
}