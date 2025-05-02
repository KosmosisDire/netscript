# Define the Engine executable
add_executable(Engine
    main.cpp
)

# --- Add dependency on the custom ScriptAPI build target ---
# Only if the target exists (i.e., using VS generator)
if(TARGET BuildScriptAPI)
    add_dependencies(Engine BuildScriptAPI)
endif()

# Link against the Core library
target_link_libraries(Engine PUBLIC Core) # PUBLIC needed for include propagation

# Define project properties for Visual Studio (optional but helpful)
set_target_properties(Engine PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>" # Add $<CONFIG> for multi-config
)

# Copy necessary runtime DLLs on Windows for easier execution
# This command should now run *after* BuildScriptAPI has finished
add_custom_command(TARGET Engine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/Core.dll"
        $<TARGET_FILE_DIR:Engine>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/ScriptAPI.dll" # Source path should be correct now
        $<TARGET_FILE_DIR:Engine>
    COMMENT "Copying dependent DLLs to Engine output directory for $<CONFIG>"
    VERBATIM
)