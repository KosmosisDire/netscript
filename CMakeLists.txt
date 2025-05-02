cmake_minimum_required(VERSION 3.20)

project(MyEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Define output directories globally
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_PDB_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin) # Keep PDBs with binaries

# Set configuration types if not set by generator (e.g., for VS Code Makefiles)
if(NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_CONFIGURATION_TYPES Debug Release RelWithDebInfo MinSizeRel CACHE STRING "Configurations" FORCE)
  if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build" FORCE)
  endif()
endif()

# Add flags for MSVC builds
if(MSVC)
  add_compile_options(/FS)
  if(NOT CMAKE_CXX_FLAGS_DEBUG)
    set(CMAKE_CXX_FLAGS_DEBUG "/Zi /Od /RTC1")
  else()
     string(APPEND CMAKE_CXX_FLAGS_DEBUG " /Zi")
  endif()
  if(NOT CMAKE_SHARED_LINKER_FLAGS_DEBUG)
      set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "/DEBUG /INCREMENTAL")
  else()
      string(APPEND CMAKE_SHARED_LINKER_FLAGS_DEBUG " /DEBUG")
  endif()
  if(NOT CMAKE_EXE_LINKER_FLAGS_DEBUG)
       set(CMAKE_EXE_LINKER_FLAGS_DEBUG "/DEBUG /INCREMENTAL")
  else()
      string(APPEND CMAKE_EXE_LINKER_FLAGS_DEBUG " /DEBUG")
  endif()
endif()

# Add Core subdirectory first, so the target is defined
add_subdirectory(Core)

# --- Custom Target to Build ScriptAPI using MSBuild ---
if(CMAKE_GENERATOR MATCHES "Visual Studio")
    find_program(MSBUILD_EXECUTABLE msbuild)
    if(NOT MSBUILD_EXECUTABLE)
        message(FATAL_ERROR "MSBuild not found. Cannot build ScriptAPI.vcxproj.")
    endif()

    set(SCRIPTAPI_PROJECT_FILE ${CMAKE_SOURCE_DIR}/ScriptAPI/ScriptAPI.vcxproj)

    add_custom_target(BuildScriptAPI ALL
        COMMAND ${MSBUILD_EXECUTABLE} ${SCRIPTAPI_PROJECT_FILE} /p:Configuration=$<CONFIG> /p:Platform=x64
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/ScriptAPI
        COMMENT "Building ScriptAPI.vcxproj for $<CONFIG>"
        VERBATIM
        # Add dependency on the Core target
        DEPENDS Core
    )
    message(STATUS "Added custom target BuildScriptAPI using MSBuild: ${MSBUILD_EXECUTABLE}")
else()
    message(WARNING "Visual Studio generator not used. ScriptAPI.vcxproj must be built manually.")
endif()

# Add Engine subdirectory
# add_subdirectory(ScriptAPI) # Stays REMOVED
add_subdirectory(Engine)