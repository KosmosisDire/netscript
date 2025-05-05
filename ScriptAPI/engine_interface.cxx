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

#include <iostream> // For std::cerr if needed

namespace ScriptAPI
{
    // Static members initialized in header

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
            scriptLoadContext = gcnew AssemblyLoadContext("ManagedScriptsContext", true);
            String^ assemblyPath = "ManagedScripts.dll";

            if (!File::Exists(assemblyPath))
            {
                Console::Error->WriteLine(String::Format("[ScriptAPI] Error: ManagedScripts.dll not found: {0}", assemblyPath));
                scriptLoadContext = nullptr; return false;
            }

            FileStream^ fs = File::Open(assemblyPath, FileMode::Open, FileAccess::Read, FileShare::Read);
            scriptAssembly = scriptLoadContext->LoadFromStream(fs);
            fs->Close();

            if (scriptAssembly == nullptr)
            {
                Console::Error->WriteLine(String::Format("[ScriptAPI] Error: Failed to load assembly: {0}", assemblyPath));
                scriptLoadContext = nullptr; return false;
            }

            Console::WriteLine(String::Format("[ScriptAPI] Successfully loaded assembly: {0}", scriptAssembly->FullName));

            DiscoverScriptTypes();
            // Use gcnew with explicit template args for Dictionary
            activeScripts = gcnew Dictionary<int, List<Script^>^>();
            isInitialized = true;
            return true;
        }
        catch (Exception^ e)
        {
            Console::Error->WriteLine(String::Format("[ScriptAPI] Exception during initialization: {0}", e->Message));
            Console::Error->WriteLine(e->StackTrace);
            scriptAssembly = nullptr; scriptLoadContext = nullptr; activeScripts = nullptr;
            isInitialized = false; return false;
        }
    }

    // --- Implementation of the static predicate ---
    bool EngineInterface::IsConcreteScript(Type^ type)
    {
        // Check if it's a non-abstract subclass of ScriptAPI::Script
        return type != nullptr && type->IsSubclassOf(Script::typeid) && !type->IsAbstract;
    }

    void EngineInterface::DiscoverScriptTypes()
    {
        // Use gcnew with explicit template args for Dictionary
        availableScriptTypes = gcnew Dictionary<String^, Type^>();
        if (scriptAssembly == nullptr) return;

        Console::WriteLine("[ScriptAPI] Discovering script types...");
        try
        {
            // Explicitly cast the array returned by GetExportedTypes() to IEnumerable<Type^>
            IEnumerable<Type^>^ typesInAssembly = safe_cast<IEnumerable<Type^>^>(scriptAssembly->GetExportedTypes());

            // Use the static predicate function with Enumerable::Where on the IEnumerable
            IEnumerable<Type^>^ discoveredTypes = Enumerable::Where(
                typesInAssembly, // Pass the correctly typed IEnumerable
                gcnew Func<Type^, bool>(&EngineInterface::IsConcreteScript) // Pass delegate to static function
            );

            // Check if discoveredTypes is null before iterating
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
        // Ensure activeScripts is not null before accessing
        if (activeScripts == nullptr) {
            activeScripts = gcnew Dictionary<int, List<Script^>^>();
        }

        if (!activeScripts->TryGetValue(entityId, entityScripts))
        {
            // Use gcnew with explicit template args for List
            entityScripts = gcnew List<Script^>();
            activeScripts->Add(entityId, entityScripts);
        }
        return entityScripts;
    }

    bool EngineInterface::AddScript(int entityId, String^ scriptName)
    {
        if (!isInitialized || availableScriptTypes == nullptr || scriptAssembly == nullptr)
        {
            Console::Error->WriteLine("[ScriptAPI] Error: AddScript called before successful initialization.");
            return false;
        }

        scriptName = scriptName->Trim();
        Type^ scriptTypeToCreate = nullptr;
        if (!availableScriptTypes->TryGetValue(scriptName, scriptTypeToCreate))
        {
            Console::Error->WriteLine(String::Format("[ScriptAPI] Error: Script type '{0}' not found or not discovered.", scriptName));
            return false;
        }

        Console::WriteLine(String::Format("[ScriptAPI] Adding script '{0}' to Entity {1}", scriptTypeToCreate->FullName, entityId));

        try
        {
            Object^ instance = Activator::CreateInstance(scriptTypeToCreate);
            Script^ newScript = safe_cast<Script^>(instance);

            if (newScript != nullptr)
            {
                newScript->SetEntityId(entityId);
                List<Script^>^ entityScripts = GetOrCreateEntityScriptList(entityId);
                entityScripts->Add(newScript);
                Console::WriteLine(String::Format("[ScriptAPI] Script '{0}' added successfully to Entity {1}.", scriptTypeToCreate->FullName, entityId));
                return true;
            }
            else
            {
                Console::Error->WriteLine(String::Format("[ScriptAPI] Error: Failed to cast created instance to Script^ for type '{0}'.", scriptTypeToCreate->FullName));
                return false;
            }
        }
        catch (Exception^ e)
        {
            Console::Error->WriteLine(String::Format("[ScriptAPI] Exception during AddScript ('{0}'): {1}", scriptName, e->Message));
            Console::Error->WriteLine(e->StackTrace);
            return false;
        }
    }

    void EngineInterface::ExecuteStartForEntity(int entityId)
    {
        if (!isInitialized || activeScripts == nullptr) return;

        List<Script^>^ entityScripts;
        if (activeScripts->TryGetValue(entityId, entityScripts))
        {
            // Use gcnew with explicit template args for List
            auto scriptsToStart = gcnew List<Script^>(entityScripts);
            for each (Script ^ script in scriptsToStart)
            {
                if (script == nullptr) continue;
                try
                {
                    script->Start();
                }
                catch (Exception^ e)
                {
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

        // Use gcnew with explicit template args for List and KeyValuePair
        auto entitiesWithScripts = gcnew List<KeyValuePair<int, List<Script^>^>>(activeScripts);

        for each (KeyValuePair<int, List<Script^>^> pair in entitiesWithScripts)
        {
            int entityId = pair.Key;
            List<Script^>^ entityScripts = pair.Value;
            // Use gcnew with explicit template args for List
            auto scriptsToUpdate = gcnew List<Script^>(entityScripts);

            for each (Script ^ script in scriptsToUpdate)
            {
                if (script == nullptr) continue;
                try
                {
                    script->Update();
                }
                catch (Exception^ e)
                {
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
        isInitialized = false;

        if (activeScripts != nullptr) activeScripts->Clear();
        if (availableScriptTypes != nullptr) availableScriptTypes->Clear();
        activeScripts = nullptr;
        availableScriptTypes = nullptr;
        scriptAssembly = nullptr;

        if (scriptLoadContext != nullptr)
        {
            try
            {
                Console::WriteLine("[ScriptAPI] Unloading AssemblyLoadContext...");
                scriptLoadContext->Unload();
                Console::WriteLine("[ScriptAPI] AssemblyLoadContext unloaded.");
            }
            catch (Exception^ e)
            {
                Console::Error->WriteLine(String::Format("[ScriptAPI] Exception during AssemblyLoadContext.Unload(): {0}", e->Message));
            }
            scriptLoadContext = nullptr;
        }
        Console::WriteLine("[ScriptAPI] Shutdown complete.");
    }

} // namespace ScriptAPI