#include <iostream>
#include <string>
#include <vector>
#include <cstdlib> // EXIT_SUCCESS, EXIT_FAILURE
#include <thread>  // For std::this_thread::sleep_for
#include <chrono>  // For std::chrono::seconds, milliseconds
#include <Windows.h> // For GetAsyncKeyState, VK_ESCAPE, VK_SPACE

// Include Core library headers
#include "dot_net_runtime.h" // Correct include path
#include "host_utils.h"      // Correct include path

// Define the function pointer types for the managed delegates
using InitDelegate = bool(*)();
using ShutdownDelegate = void(*)();
using ReloadDelegate = bool(*)(); // Delegate for Reload
using AddScriptDelegate = bool(*)(int, const char*);
using ExecuteStartDelegate = void(*)(int);
using ExecuteUpdateDelegate = void(*)();

// Simple state tracking for hot reload
struct ScriptInstanceInfo {
    int entityId;
    std::string scriptName;
    // Add other config/state if needed
};

// Helper function to add a script and execute its start method
bool AddAndStartScript(
    AddScriptDelegate addFunc,
    ExecuteStartDelegate startFunc,
    const ScriptInstanceInfo& info)
{
    if (!addFunc || !startFunc) return false;

    std::cout << "Attempting to add script '" << info.scriptName << "' to entity " << info.entityId << "..." << std::endl;
    bool added = false;
    try { added = addFunc(info.entityId, info.scriptName.c_str()); }
    catch (...) { std::cerr << "!!! Exception caught calling AddScript delegate." << std::endl; return false; }

    if (added) {
        std::cout << "Script added. Executing Start() for entity " << info.entityId << "..." << std::endl;
        try { startFunc(info.entityId); }
        catch (...) { std::cerr << "!!! Exception caught calling ExecuteStart delegate." << std::endl; /* Continue? */ }
        return true;
    } else {
        std::cerr << "Failed to add script '" << info.scriptName << "'." << std::endl;
        return false;
    }
}

int main()
{
    std::cout << "Engine starting..." << std::endl;

    // --- Initialization Steps (Same as before) ---
    std::cout << "Searching for .NET 9+ runtime..." << std::endl;
    const int requiredMajorVersion = 9;
    std::string runtimePath = Core::HostUtils::find_latest_dot_net_runtime(requiredMajorVersion);
    if (runtimePath.empty()) { std::cerr << "Error: .NET Runtime not found." << std::endl; return EXIT_FAILURE; }
    std::cout << "Found .NET Runtime at: " << runtimePath << std::endl;

    std::string appBasePath = Core::HostUtils::get_current_executable_directory();
    if (appBasePath.empty()) { std::cerr << "Error: Cannot get app base path." << std::endl; return EXIT_FAILURE; }
    std::cout << "Application Base Path: " << appBasePath << std::endl;

    std::cout << "Building TPA list..." << std::endl;
    std::string tpaList = Core::HostUtils::build_tpa_list(runtimePath);
    tpaList += Core::HostUtils::build_tpa_list(appBasePath);
    if (tpaList.empty()) { std::cerr << "Warning: TPA list is empty." << std::endl; }
    else { std::cout << "TPA list built." << std::endl; }

    std::cout << "Initializing CoreCLR..." << std::endl;
    Core::DotNetRuntime runtime;
    bool initialized = runtime.initialize(runtimePath, appBasePath, tpaList);
    if (!initialized) { std::cerr << "Failed to initialize .NET runtime." << std::endl; return EXIT_FAILURE; }
    std::cout << "CoreCLR Initialized successfully!" << std::endl;

    // --- Get Delegates for ScriptAPI ---
    std::cout << "Getting delegates from ScriptAPI..." << std::endl;
    InitDelegate scriptApiInit = nullptr;
    ShutdownDelegate scriptApiShutdown = nullptr;
    ReloadDelegate scriptApiReload = nullptr; // Get Reload delegate
    AddScriptDelegate scriptApiAddScript = nullptr;
    ExecuteStartDelegate scriptApiExecuteStart = nullptr;
    ExecuteUpdateDelegate scriptApiExecuteUpdate = nullptr;

    bool delegatesOk = true;
    delegatesOk &= runtime.create_delegate("ScriptAPI", "ScriptAPI.EngineInterface", "Init", &scriptApiInit);
    delegatesOk &= runtime.create_delegate("ScriptAPI", "ScriptAPI.EngineInterface", "Shutdown", &scriptApiShutdown);
    delegatesOk &= runtime.create_delegate("ScriptAPI", "ScriptAPI.EngineInterface", "Reload", &scriptApiReload); // Add Reload
    delegatesOk &= runtime.create_delegate("ScriptAPI", "ScriptAPI.EngineInterface", "AddScript", &scriptApiAddScript);
    delegatesOk &= runtime.create_delegate("ScriptAPI", "ScriptAPI.EngineInterface", "ExecuteStartForEntity", &scriptApiExecuteStart);
    delegatesOk &= runtime.create_delegate("ScriptAPI", "ScriptAPI.EngineInterface", "ExecuteUpdate", &scriptApiExecuteUpdate);

    if (!delegatesOk || !scriptApiInit || !scriptApiShutdown || !scriptApiReload || !scriptApiAddScript || !scriptApiExecuteStart || !scriptApiExecuteUpdate) {
         std::cerr << "Failed to get one or more required delegates from ScriptAPI." << std::endl;
         runtime.shutdown(); return EXIT_FAILURE;
    }
     std::cout << "Delegates obtained successfully." << std::endl;

    // --- Initialize ScriptAPI Environment ---
    std::cout << "Calling ScriptAPI Init..." << std::endl;
    bool scriptApiInitialized = false;
    try { scriptApiInitialized = scriptApiInit(); }
    catch (...) { std::cerr << "!!! Exception caught calling ScriptAPI Init delegate." << std::endl; }
    if (!scriptApiInitialized) {
        std::cerr << "ScriptAPI initialization failed." << std::endl;
        runtime.shutdown(); return EXIT_FAILURE;
    }
    std::cout << "ScriptAPI Init completed." << std::endl;

    // --- Keep track of scripts to re-add after reload ---
    std::vector<ScriptInstanceInfo> activeScriptInstances;
    activeScriptInstances.push_back({0, "MyFirstScript"}); // Add our initial script info

    // --- Initial Script Loading ---
    for(const auto& scriptInfo : activeScriptInstances) {
         AddAndStartScript(scriptApiAddScript, scriptApiExecuteStart, scriptInfo);
    }

    // --- Main Engine Loop ---
    std::cout << "\nStarting main loop (Press SPACE to Reload, ESC to Exit)..." << std::endl;
    bool running = true;
    int frameCount = 0;
    bool spacePressedLastFrame = false; // To detect key press edge

    while(running)
    {
        // --- Input Handling ---
        bool escapePressed = GetAsyncKeyState(VK_ESCAPE) & 0x8000;
        bool spacePressed = GetAsyncKeyState(VK_SPACE) & 0x8000;

        if (escapePressed) {
            running = false;
            std::cout << "\nESC pressed, exiting loop." << std::endl;
            continue;
        }

        // Check for Space press *edge* (pressed now, but not last frame)
        if (spacePressed && !spacePressedLastFrame) {
            std::cout << "\n--- HOT RELOAD REQUESTED ---" << std::endl;

            // 1. (Manual Step) Rebuild ManagedScripts.dll
            // In a real engine, you might trigger this automatically or watch for changes.
            // For now, you need to manually run `dotnet build` in ManagedScripts
            // *before* pressing Space.
            std::cout << ">>> Please ensure ManagedScripts.dll has been rebuilt <<<" << std::endl;
            std::cout << ">>> Press SPACE again to confirm reload... <<<" << std::endl;

            // Simple confirmation mechanism - requires pressing space twice
            // Wait for space release then press again
            while (GetAsyncKeyState(VK_SPACE) & 0x8000) { Sleep(10); } // Wait for release
            while (!(GetAsyncKeyState(VK_SPACE) & 0x8000)) { // Wait for press again
                if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) { // Allow exit during wait
                     running = false; break;
                }
                Sleep(10);
            }
             while (GetAsyncKeyState(VK_SPACE) & 0x8000) { Sleep(10); } // Wait for release again
            if (!running) continue; // Check if Escape was pressed during wait

            std::cout << "--- Reloading .NET Scripts ---" << std::endl;
            bool reloadOk = false;
            try { reloadOk = scriptApiReload(); }
            catch(...) { std::cerr << "!!! Exception caught calling ScriptAPI Reload delegate." << std::endl; }

            if (reloadOk) {
                std::cout << "--- Re-adding script instances ---" << std::endl;
                // Re-add all previously active scripts
                for(const auto& scriptInfo : activeScriptInstances) {
                    AddAndStartScript(scriptApiAddScript, scriptApiExecuteStart, scriptInfo);
                }
                std::cout << "--- Hot Reload Complete ---" << std::endl;
            } else {
                 std::cerr << "--- Hot Reload FAILED ---" << std::endl;
                 // Maybe stop the engine or prevent further updates?
                 // For now, just log the error. Update loop will continue using old state if reload failed badly.
            }
        }
        spacePressedLastFrame = spacePressed; // Update state for next frame

        // --- Execute Script Updates ---
        try { scriptApiExecuteUpdate(); }
        catch (...) { std::cerr << "!!! Exception caught calling ScriptAPI ExecuteUpdate delegate." << std::endl; }

        // Simulate frame delay
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
        frameCount++;
    }
    std::cout << "Exited main loop after " << frameCount << " frames." << std::endl;

    // --- Shutdown ScriptAPI ---
    std::cout << "Calling ScriptAPI Shutdown..." << std::endl;
    try { scriptApiShutdown(); }
    catch (...) { std::cerr << "!!! Exception caught calling ScriptAPI Shutdown delegate." << std::endl; }

    // --- Shutdown CoreCLR ---
    std::cout << "Shutting down CoreCLR..." << std::endl;
    bool shutdownSuccess = runtime.shutdown();
    if (!shutdownSuccess) { std::cerr << "CoreCLR shutdown reported an error." << std::endl; }
    else { std::cout << "CoreCLR shutdown successful." << std::endl; }

    std::cout << "Engine exiting." << std::endl;
    return EXIT_SUCCESS;
}