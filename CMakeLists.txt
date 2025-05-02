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
  # Set default build type if none was specified
  if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build" FORCE)
  endif()
endif()

# Add flags for MSVC builds
if(MSVC)
  # Add /FS flag globally to handle parallel PDB writes during compilation
  add_compile_options(/FS)

  # Explicitly set Debug Information Format options for Debug config
  # This ensures /Zi for compilation and /DEBUG for linking
  # which helps with PDB generation and finding by the linker/debugger.
  # Check if variables exist before appending, avoiding potential issues
  if(NOT CMAKE_CXX_FLAGS_DEBUG)
    set(CMAKE_CXX_FLAGS_DEBUG "/Zi /Od /RTC1") # Base Debug flags
  else()
     string(APPEND CMAKE_CXX_FLAGS_DEBUG " /Zi") # Ensure /Zi is present
  endif()

  if(NOT CMAKE_SHARED_LINKER_FLAGS_DEBUG)
      set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "/DEBUG /INCREMENTAL") # Base shared linker debug flags
  else()
      string(APPEND CMAKE_SHARED_LINKER_FLAGS_DEBUG " /DEBUG") # Ensure /DEBUG is present
  endif()

  if(NOT CMAKE_EXE_LINKER_FLAGS_DEBUG)
       set(CMAKE_EXE_LINKER_FLAGS_DEBUG "/DEBUG /INCREMENTAL") # Base exe linker debug flags
  else()
      string(APPEND CMAKE_EXE_LINKER_FLAGS_DEBUG " /DEBUG") # Ensure /DEBUG is present
  endif()

endif()

# Add subdirectories
add_subdirectory(Core)
add_subdirectory(ScriptAPI)
add_subdirectory(Engine)