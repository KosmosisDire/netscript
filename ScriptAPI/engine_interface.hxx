#pragma once

// Use Managed C++ namespaces
using namespace System;
using namespace System::Reflection; // For Assembly, MethodInfo
using namespace System::IO;         // For File, FileStream etc.
using namespace System::Runtime::Loader; // For AssemblyLoadContext

namespace ScriptAPI
{
    public ref class EngineInterface
    {
    public:
        // Initializes the scripting environment (loads assemblies)
        // Returns true on success, false otherwise.
        static bool Init();

        // Calls the static SayHello method in ManagedScripts.MyFirstScript
        static void CallSayHello();

        // Simple ping method (kept for initial testing if desired, can be removed)
        static void Ping();

    private:
        // Initialize static members inline within the class definition
        static AssemblyLoadContext^ scriptLoadContext = nullptr;
        static Assembly^ scriptAssembly = nullptr;
        static bool isInitialized = false;
    };
} // namespace ScriptAPI