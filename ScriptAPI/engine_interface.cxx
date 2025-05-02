#include "engine_interface.hxx"

// Include iostream via C++/CLI's Console for managed output
#include <iostream> // We can also use standard iostream if needed

namespace ScriptAPI
{
    void EngineInterface::Ping()
    {
        // Use System.Console for output in managed code
        Console::WriteLine("ScriptAPI::EngineInterface::Ping() called from C++!");

        // Alternatively, you could use std::cout here as well in mixed mode
        // std::cout << "ScriptAPI::EngineInterface::Ping() called from C++! (std::cout)" << std::endl;
    }
} // namespace ScriptAPI