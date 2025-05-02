# Define the Core library
add_library(Core SHARED
    import_export.h
    host_utils.h
    host_utils.cpp
    dot_net_runtime.h
    dot_net_runtime.cpp
)

# Add include directories
target_include_directories(Core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR} # Allows Core headers to be included using "Core/header.h"
    ${CMAKE_SOURCE_DIR}/extern/dotnet/include # Include path for coreclrhost.h
)

# Define the DLL export macro
target_compile_definitions(Core PRIVATE DLL_API_EXPORT)

# Link necessary Windows libraries
target_link_libraries(Core PRIVATE Shlwapi.lib)

# Define project properties for Visual Studio (optional but helpful)
set_target_properties(Core PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
)

# Ensure PDB files are copied for Debug builds
if(MSVC)
    set_target_properties(Core PROPERTIES COMPILE_PDB_OUTPUT_DIRECTORY ${CMAKE_PDB_OUTPUT_DIRECTORY})
    set_target_properties(Core PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
endif()