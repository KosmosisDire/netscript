# Define the ScriptAPI library (C++/CLI)
add_library(ScriptAPI SHARED
    engine_interface.hxx
    engine_interface.cxx
)

# Enable C++/CLI support (requires Visual Studio Generator)
set_target_properties(ScriptAPI PROPERTIES COMMON_LANGUAGE_RUNTIME "")

# --- Specify Target Framework ---
# Option A: Try setting the TargetFramework property directly for VS Generators
# This is often used for C# projects, let's see if it works for C++/CLI in modern VS
if(CMAKE_GENERATOR MATCHES "Visual Studio")
    set_target_properties(ScriptAPI PROPERTIES VS_GLOBAL_TargetFramework "net9.0") # Use the TFM string
endif()

# Option B (If Option A fails): Remove the framework setting attempts from CMake
# Remove or comment out: set_target_properties(ScriptAPI PROPERTIES VS_DOTNET_TARGET_FRAMEWORK_VERSION "net9.0")
# Remove or comment out: if(CMAKE_GENERATOR MATCHES "Visual Studio") ... endif() section above
# We might need to add VS_DOTNET_REFERENCE properties manually in this case.

# Link against the Core library (needed for includes and potential callbacks)
target_link_libraries(ScriptAPI PRIVATE Core)

# Add include directories
target_include_directories(ScriptAPI PRIVATE
    ${CMAKE_SOURCE_DIR}/Core # Include Core headers
    ${CMAKE_SOURCE_DIR}/extern/dotnet/include # Include coreclrhost.h (if needed directly)
)

# C++ Standard
set_target_properties(ScriptAPI PROPERTIES CXX_STANDARD ${CMAKE_CXX_STANDARD})
set_target_properties(ScriptAPI PROPERTIES CXX_STANDARD_REQUIRED ${CMAKE_CXX_STANDARD_REQUIRED})

# Ensure output directories match the rest of the project
set_target_properties(ScriptAPI PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}"
    PDB_OUTPUT_DIRECTORY "${CMAKE_PDB_OUTPUT_DIRECTORY}"
)

# Define project properties for Visual Studio (optional but helpful)
set_target_properties(ScriptAPI PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
)

# Ensure PDB files are copied for Debug builds
if(MSVC)
    set_target_properties(ScriptAPI PROPERTIES COMPILE_PDB_OUTPUT_DIRECTORY ${CMAKE_PDB_OUTPUT_DIRECTORY})
endif()

# Optional: Manually add references if needed (especially if Option A fails)
# Example (uncomment and adjust if needed):
# set_target_properties(ScriptAPI PROPERTIES
#     VS_DOTNET_REFERENCE_System_Runtime "System.Runtime.dll"
#     VS_DOTNET_REFERENCE_System_Console "System.Console.dll"
#     # Add others like System.Collections if required by your C++/CLI code
# )