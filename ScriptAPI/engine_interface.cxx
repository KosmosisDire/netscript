#include "pch.h" // Add this line FIRST
#include "engine_interface.hxx"

// System namespaces are brought in via the header's 'using' directives OR pch.h

#include <iostream>

namespace ScriptAPI
{
    // NOTE: Static member initializations are IN THE HEADER (.hxx) file now

    bool EngineInterface::Init()
    {
        if (isInitialized)
        {
            Console::WriteLine("[ScriptAPI] Already initialized.");
            return true;
        }

        Console::WriteLine("[ScriptAPI] Initializing...");

        try
        {
            // Create a collectible AssemblyLoadContext
            scriptLoadContext = gcnew AssemblyLoadContext("ManagedScriptsContext", true);

            String^ assemblyPath = "ManagedScripts.dll"; // Relative path

            if (!File::Exists(assemblyPath))
            {
                Console::Error->WriteLine("[ScriptAPI] Error: ManagedScripts.dll not found at path: " + assemblyPath);
                scriptLoadContext = nullptr;
                return false;
            }

            // FileStream requires System::IO namespace (included via header/pch)
            FileStream^ fs = File::Open(assemblyPath, FileMode::Open, FileAccess::Read, FileShare::Read);
            scriptAssembly = scriptLoadContext->LoadFromStream(fs);
            fs->Close(); // Close the stream after loading

            if (scriptAssembly == nullptr)
            {
                Console::Error->WriteLine("[ScriptAPI] Error: Failed to load assembly from stream: " + assemblyPath);
                if (scriptLoadContext != nullptr) {
                    scriptLoadContext = nullptr;
                }
                return false;
            }

            Console::WriteLine("[ScriptAPI] Successfully loaded assembly: " + scriptAssembly->FullName);
            isInitialized = true;
            return true;
        }
        catch (Exception^ e)
        {
            Console::Error->WriteLine("[ScriptAPI] Exception during initialization: " + e->Message);
            Console::Error->WriteLine(e->StackTrace);

            scriptAssembly = nullptr;
            scriptLoadContext = nullptr; // Ensure context is null on failure
            isInitialized = false;
            return false;
        }
    }

    void EngineInterface::CallSayHello()
    {
        if (!isInitialized || scriptAssembly == nullptr)
        {
            Console::Error->WriteLine("[ScriptAPI] Error: CallSayHello called before successful initialization.");
            return;
        }

        try
        {
            // Type requires System namespace (via header/pch)
            Type^ scriptType = scriptAssembly->GetType("ManagedScripts.MyFirstScript");

            if (scriptType == nullptr)
            {
                Console::Error->WriteLine("[ScriptAPI] Error: Could not find type 'ManagedScripts.MyFirstScript' in assembly.");
                return;
            }

            // MethodInfo requires System::Reflection namespace (via header/pch)
            MethodInfo^ method = scriptType->GetMethod("SayHello", BindingFlags::Public | BindingFlags::Static);

            if (method == nullptr)
            {
                Console::Error->WriteLine("[ScriptAPI] Error: Could not find static method 'SayHello' in type 'ManagedScripts.MyFirstScript'.");
                return;
            }

            // Object requires System namespace (via header/pch)
            array<Object^>^ args = gcnew array<Object^>(1);
            args[0] = "Native World via C++/CLI";

            Console::WriteLine("[ScriptAPI] Invoking ManagedScripts.MyFirstScript.SayHello...");
            method->Invoke(nullptr, args);
            Console::WriteLine("[ScriptAPI] Invocation complete.");

        }
        catch (Exception^ e)
        {
            Console::Error->WriteLine("[ScriptAPI] Exception during CallSayHello: " + e->Message);
            Console::Error->WriteLine(e->StackTrace);
        }
    }

    void EngineInterface::Ping()
    {
        Console::WriteLine("[ScriptAPI] EngineInterface::Ping() called!");
    }

} // namespace ScriptAPI