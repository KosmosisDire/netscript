# Define the Engine executable
add_executable(Engine
    main.cpp
)

# Link against the Core library
target_link_libraries(Engine PUBLIC Core) # PUBLIC needed for include propagation

# Define project properties for Visual Studio (optional but helpful)
set_target_properties(Engine PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>" # Add $<CONFIG> for multi-config
)

# Ensure PDB files are copied for Debug builds (matches Core settings)
if(MSVC)
    # Let CMake manage PDB paths for targets. Output PDBs next to targets.
    set(CMAKE_DEBUG_POSTFIX "d") # Optional: Add 'd' postfix for debug libraries/executables
    # The PDB output directory settings might conflict with /FS or standard VS behavior
    # Let's remove them and rely on /FS and standard CMake/VS PDB handling.
    # set_target_properties(Engine PROPERTIES COMPILE_PDB_OUTPUT_DIRECTORY ${CMAKE_PDB_OUTPUT_DIRECTORY}/$<CONFIG>)
    # set_target_properties(Engine PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>)
endif()

# Copy necessary runtime DLLs on Windows for easier execution
# This version uses generator expressions directly in the command
# and should work for both single and multi-config generators.
add_custom_command(TARGET Engine POST_BUILD
    # Use $<CONFIG> generator expression to get the configuration-specific path
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/Core.dll" # Source Core.dll
        $<TARGET_FILE_DIR:Engine> # Destination is Engine's output dir
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/ScriptAPI.dll" # Source ScriptAPI.dll
        $<TARGET_FILE_DIR:Engine> # Destination is Engine's output dir
    # No CONFIGURATIONS needed here, it runs per config anyway
    COMMENT "Copying dependent DLLs to Engine output directory for $<CONFIG>"
    VERBATIM # Often needed with generator expressions in commands
)