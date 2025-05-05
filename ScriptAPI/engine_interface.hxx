#pragma once

#include "script.hxx" // Include the base script class definition

// Use Managed C++ namespaces
using namespace System;
using namespace System::Reflection;
using namespace System::IO;
using namespace System::Runtime::Loader;
using namespace System::Collections::Generic; // For List<>, Dictionary<>

namespace ScriptAPI
{
    public ref class EngineInterface
    {
    public:
        static bool Init();
        static bool AddScript(int entityId, String^ scriptName);
        static void ExecuteStartForEntity(int entityId);
        static void ExecuteUpdate();
        // --- New Method ---
        // Reloads the script assembly and re-initializes script types.
        static bool Reload();
        // --- End New Method ---
        static void Shutdown();

    private:
        static void DiscoverScriptTypes();
        static List<Script^>^ GetOrCreateEntityScriptList(int entityId);
        static bool IsConcreteScript(Type^ type);

        // --- Helper for cleanup ---
        static void ClearScriptData();
        // --- Helper for assembly loading/discovery ---
        static bool LoadAndDiscoverScripts();


        // --- Static Members ---
        static AssemblyLoadContext^ scriptLoadContext = nullptr;
        static Assembly^ scriptAssembly = nullptr;
        static bool isInitialized = false;
        static Dictionary<String^, Type^>^ availableScriptTypes = nullptr;
        static Dictionary<int, List<Script^>^>^ activeScripts = nullptr;
    };
} // namespace ScriptAPI