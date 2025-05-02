#pragma once

// Use Managed C++ namespaces
using namespace System;

namespace ScriptAPI
{
    // public ref class: Declares a managed class visible outside the assembly
    public ref class EngineInterface
    {
    public:
        // Static method callable from native code via coreclr_create_delegate
        static void Ping();

    // Add private members or methods if needed later
    // private:
    };
} // namespace ScriptAPI