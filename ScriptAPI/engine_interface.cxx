#include "pch.h"

// --- Add explicit assembly references ---
#using <System.Runtime.dll>
#using <System.IO.FileSystem.dll>
#using <System.Linq.dll> // For Enumerable
#using <System.Reflection.dll>
#using <System.Collections.dll>

#include "engine_interface.hxx"

// Additional using directives needed
using namespace System::Linq; // For Enumerable class
using namespace System::Threading; // For Thread::Sleep

#include <iostream> // For std::cerr if needed

namespace ScriptAPI
{
    // Static members initialized in header

    // Helper function to clear script-related data structures
    void EngineInterface::ClearScriptData()
    {
        Console::WriteLine("[ScriptAPI] Clearing script data...");
        isInitialized = false; // Mark as uninitialized during cleanup/reload

        if (activeScripts != nullptr) activeScripts->Clear();
        if (availableScriptTypes != nullptr) availableScriptTypes->Clear();
        activeScripts = nullptr;
        availableScriptTypes = nullptr;
        scriptAssembly = nullptr; // Release reference to the assembly
    }

    // Helper function to load assembly and discover scripts
    // Used by Init and Reload
    bool EngineInterface::LoadAndDiscoverScripts()
    {
        try
        {
            // Ensure a valid context exists or create a new one
            if (scriptLoadContext == nullptr) {
                scriptLoadContext = gcnew AssemblyLoadContext("ManagedScriptsContext", true);
                Console::WriteLine("[ScriptAPI] Created new AssemblyLoadContext.");
            }

            String^ assemblyPath = "ManagedScripts.dll";

            if (!File::Exists(assemblyPath))
            {
                Console::Error->WriteLine(String::Format("[ScriptAPI] Error: ManagedScripts.dll not found: {0}", assemblyPath));
                // Don't nullify scriptLoadContext here, it might be needed for unload later
                return false;
            }

            // Load into the current context
            FileStream^ fs = File::Open(assemblyPath, FileMode::Open, FileAccess::Read, FileShare::Read);
            scriptAssembly = scriptLoadContext->LoadFromStream(fs);
            fs->Close();

            if (scriptAssembly == nullptr)
            {
                Console::Error->WriteLine(String::Format("[ScriptAPI] Error: Failed to load assembly: {0}", assemblyPath));
                return false; // Context remains for potential unload
            }

            Console::WriteLine(String::Format("[ScriptAPI] Successfully loaded assembly: {0}", scriptAssembly->FullName));

            DiscoverScriptTypes(); // Discover types from the newly loaded assembly
            activeScripts = gcnew Dictionary<int, List<Script^>^>(); // Reset active scripts

            return true;
        }
        catch (Exception^ e)
        {
            Console::Error->WriteLine(String::Format("[ScriptAPI] Exception during LoadAndDiscoverScripts: {0}", e->Message));
            Console::Error->WriteLine(e->StackTrace);
            scriptAssembly = nullptr; // Ensure assembly ref is null on error
            // Don't nullify context, allow unload attempt
            return false;
        }
    }


    bool EngineInterface::Init()
    {
        if (isInitialized)
        {
            Console::WriteLine("[ScriptAPI] Already initialized.");
            return true;
        }
        Console::WriteLine("[ScriptAPI] Initializing...");

        // Perform initial load and discovery
        if (LoadAndDiscoverScripts()) {
            isInitialized = true; // Mark as initialized only on success
            return true;
        }
        else {
            // Cleanup potentially partially created context if loading failed
            Shutdown(); // Call full shutdown to ensure cleanup
            return false;
        }
    }

    // --- Implementation of Reload ---
    bool EngineInterface::Reload()
    {
        Console::WriteLine("[ScriptAPI] Reload requested...");

        // 1. Clear existing script instances and type lookups
        ClearScriptData();

        // 2. Unload the existing AssemblyLoadContext
        if (scriptLoadContext != nullptr)
        {
            try
            {
                Console::WriteLine("[ScriptAPI] Unloading previous AssemblyLoadContext...");
                scriptLoadContext->Unload();
                Console::WriteLine("[ScriptAPI] Unload initiated.");

                // Give the unload and GC some time. This is not foolproof but helps.
                // WaitForPendingFinalizers can block indefinitely if something holds on.
                // A loop with GC.Collect() and Sleep might be safer in complex scenarios.
                scriptLoadContext = nullptr; // Release our reference
                GC::Collect();
                GC::WaitForPendingFinalizers(); // Wait for finalizers to run
                GC::Collect(); // Collect again after finalizers
                Console::WriteLine("[ScriptAPI] GC finalized after unload attempt.");

            }
            catch (Exception^ e)
            {
                // Log error but continue - context might be partially unloaded or stuck
                Console::Error->WriteLine(String::Format("[ScriptAPI] Exception during AssemblyLoadContext.Unload() or GC: {0}", e->Message));
                scriptLoadContext = nullptr; // Ensure reference is cleared anyway
            }
        }
        else {
            Console::WriteLine("[ScriptAPI] No previous AssemblyLoadContext to unload.");
        }


        // 3. Re-load the assembly and re-discover scripts
        Console::WriteLine("[ScriptAPI] Reloading scripts...");
        if (LoadAndDiscoverScripts()) {
            isInitialized = true; // Mark as initialized again
            Console::WriteLine("[ScriptAPI] Reload complete.");
            return true;
        }
        else {
            Console::Error->WriteLine("[ScriptAPI] Failed to reload scripts.");
            // Cleanup after failed reload attempt
            Shutdown(); // Call full shutdown
            return false;
        }
    }


    // --- Implementation of the static predicate ---
    bool EngineInterface::IsConcreteScript(Type^ type)
    {
        return type != nullptr && type->IsSubclassOf(Script::typeid) && !type->IsAbstract;
    }

    void EngineInterface::DiscoverScriptTypes()
    {
        availableScriptTypes = gcnew Dictionary<String^, Type^>();
        if (scriptAssembly == nullptr) return;

        Console::WriteLine("[ScriptAPI] Discovering script types...");
        try
        {
            IEnumerable<Type^>^ typesInAssembly = safe_cast<IEnumerable<Type^>^>(scriptAssembly->GetExportedTypes());
            IEnumerable<Type^>^ discoveredTypes = Enumerable::Where(
                typesInAssembly,
                gcnew Func<Type^, bool>(&EngineInterface::IsConcreteScript)
            );

            if (discoveredTypes == nullptr) {
                Console::Error->WriteLine("[ScriptAPI] Error: Enumerable::Where returned null during script discovery.");
                return;
            }

            for each (Type ^ type in discoveredTypes)
            {
                Console::WriteLine(String::Format("[ScriptAPI]   Found script: {0}", type->FullName));
                if (!availableScriptTypes->ContainsKey(type->FullName))
                {
                    availableScriptTypes->Add(type->FullName, type);
                }
                if (!availableScriptTypes->ContainsKey(type->Name) && type->Name != type->FullName)
                {
                    availableScriptTypes->Add(type->Name, type);
                    Console::WriteLine(String::Format("[ScriptAPI]     (Also mapped by name: {0})", type->Name));
                }
            }
            Console::WriteLine(String::Format("[ScriptAPI] Discovered {0} unique script type mappings.", availableScriptTypes->Count));
        }
        catch (Exception^ e)
        {
            Console::Error->WriteLine(String::Format("[ScriptAPI] Exception during script discovery: {0}", e->Message));
            availableScriptTypes->Clear();
        }
    }

    List<Script^>^ EngineInterface::GetOrCreateEntityScriptList(int entityId)
    {
        List<Script^>^ entityScripts;
        if (activeScripts == nullptr) {
            activeScripts = gcnew Dictionary<int, List<Script^>^>();
        }
        if (!activeScripts->TryGetValue(entityId, entityScripts))
        {
            entityScripts = gcnew List<Script^>();
            activeScripts->Add(entityId, entityScripts);
        }
        return entityScripts;
    }

    bool EngineInterface::AddScript(int entityId, String^ scriptName)
    {
        if (!isInitialized || availableScriptTypes == nullptr || scriptAssembly == nullptr) {
            Console::Error->WriteLine("[ScriptAPI] Error: AddScript called before successful initialization/reload.");
            return false;
        }

        scriptName = scriptName->Trim();
        Type^ scriptTypeToCreate = nullptr;
        if (!availableScriptTypes->TryGetValue(scriptName, scriptTypeToCreate)) {
            Console::Error->WriteLine(String::Format("[ScriptAPI] Error: Script type '{0}' not found or not discovered.", scriptName));
            return false;
        }

        Console::WriteLine(String::Format("[ScriptAPI] Adding script '{0}' to Entity {1}", scriptTypeToCreate->FullName, entityId));

        try {
            Object^ instance = Activator::CreateInstance(scriptTypeToCreate);
            Script^ newScript = safe_cast<Script^>(instance);

            if (newScript != nullptr) {
                newScript->SetEntityId(entityId);
                List<Script^>^ entityScripts = GetOrCreateEntityScriptList(entityId);
                entityScripts->Add(newScript);
                Console::WriteLine(String::Format("[ScriptAPI] Script '{0}' added successfully to Entity {1}.", scriptTypeToCreate->FullName, entityId));
                return true;
            }
            else {
                Console::Error->WriteLine(String::Format("[ScriptAPI] Error: Failed to cast created instance to Script^ for type '{0}'.", scriptTypeToCreate->FullName));
                return false;
            }
        }
        catch (Exception^ e) {
            Console::Error->WriteLine(String::Format("[ScriptAPI] Exception during AddScript ('{0}'): {1}", scriptName, e->Message));
            Console::Error->WriteLine(e->StackTrace);
            return false;
        }
    }

    void EngineInterface::ExecuteStartForEntity(int entityId)
    {
        if (!isInitialized || activeScripts == nullptr) return;
        List<Script^>^ entityScripts;
        if (activeScripts->TryGetValue(entityId, entityScripts)) {
            auto scriptsToStart = gcnew List<Script^>(entityScripts);
            for each (Script ^ script in scriptsToStart) {
                if (script == nullptr) continue;
                try { script->Start(); }
                catch (Exception^ e) {
                    String^ scriptTypeName = (script->GetType() != nullptr) ? script->GetType()->Name : "Unknown Script";
                    Console::Error->WriteLine(String::Format("[ScriptAPI] Exception during {0}->Start() for Entity {1}: {2}", scriptTypeName, entityId, e->Message));
                    Console::Error->WriteLine(e->StackTrace);
                }
            }
        }
    }

    void EngineInterface::ExecuteUpdate()
    {
        if (!isInitialized || activeScripts == nullptr) return;
        auto entitiesWithScripts = gcnew List<KeyValuePair<int, List<Script^>^>>(activeScripts);
        for each (KeyValuePair<int, List<Script^>^> pair in entitiesWithScripts) {
            int entityId = pair.Key;
            List<Script^>^ entityScripts = pair.Value;
            auto scriptsToUpdate = gcnew List<Script^>(entityScripts);
            for each (Script ^ script in scriptsToUpdate) {
                if (script == nullptr) continue;
                try { script->Update(); }
                catch (Exception^ e) {
                    String^ scriptTypeName = (script->GetType() != nullptr) ? script->GetType()->Name : "Unknown Script";
                    Console::Error->WriteLine(String::Format("[ScriptAPI] Exception during {0}->Update() for Entity {1}: {2}", scriptTypeName, entityId, e->Message));
                    Console::Error->WriteLine(e->StackTrace);
                }
            }
        }
    }

    void EngineInterface::Shutdown()
    {
        Console::WriteLine("[ScriptAPI] Shutting down...");
        // Clear script data first
        ClearScriptData();

        // Then unload context if it exists (might be null if Init/Reload failed)
        if (scriptLoadContext != nullptr)
        {
            try {
                Console::WriteLine("[ScriptAPI] Unloading AssemblyLoadContext on shutdown...");
                scriptLoadContext->Unload();
                scriptLoadContext = nullptr; // Clear ref immediately after calling Unload
                // Optional: Add GC calls here too if needed, similar to Reload
               // GC::Collect(); GC::WaitForPendingFinalizers(); GC::Collect();
                Console::WriteLine("[ScriptAPI] AssemblyLoadContext unload initiated on shutdown.");
            }
            catch (Exception^ e) {
                Console::Error->WriteLine(String::Format("[ScriptAPI] Exception during AssemblyLoadContext.Unload() on shutdown: {0}", e->Message));
                scriptLoadContext = nullptr; // Ensure ref is cleared
            }
        }
        Console::WriteLine("[ScriptAPI] Shutdown complete.");
    }

} // namespace ScriptAPI