cmake_minimum_required(VERSION 3.12)

project(CKBuildingBlocks VERSION 2.1.0.14 LANGUAGES C CXX)

# Determine if CKBuildingBlocks is built as a subproject (using add_subdirectory) or if it is the main project
set(CKBB_IS_MAIN_PROJECT OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(CKBB_IS_MAIN_PROJECT ON)
endif ()

# CMake options with CKBB_ prefix
option(CKBB_BUILD_ALL_MODULES "Build all available building block modules" ON)
option(CKBB_INSTALL "Generate install target" ${CKBB_IS_MAIN_PROJECT})
option(CKBB_EXPORT_COMPILE_COMMANDS "Generate compile_commands.json" ${CKBB_IS_MAIN_PROJECT})
option(CKBB_USE_FOLDERS "Use folders to organize targets in an IDE" ON)
option(VIRTOOLS_SDK_FETCH_FROM_GIT "Fetch Virtools SDK from git if not found" OFF)

set(VIRTOOLS_SDK_PATH "" CACHE PATH "Path to the Virtools SDK")
set(VIRTOOLS_SDK_FETCH_FROM_GIT_PATH "" CACHE FILEPATH "Location to download SDK")

# C++ standard
set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED NO)
set(CMAKE_CXX_EXTENSIONS YES)

# IDE organization
if (CKBB_USE_FOLDERS)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
    set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMakeTargets")
endif ()

# Platform check
if (NOT WIN32)
    message(FATAL_ERROR "CKBuildingBlocks only supports Windows.")
endif ()

# Windows-specific settings
if (WIN32)
    set(CMAKE_USE_RELATIVE_PATHS TRUE)
    set(CMAKE_SUPPRESS_REGENERATION TRUE)
endif ()

# Build type configuration (only for main project)
if (CKBB_IS_MAIN_PROJECT)
    if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
        message(STATUS "Setting build type to 'Release' as no build type was specified")
        set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the build type (Debug/Release)" FORCE)
    endif ()
endif ()

# Installation prefix (only for main project)
if (CKBB_IS_MAIN_PROJECT AND CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}/install" CACHE PATH "The root directory of the installation" FORCE)
    message(STATUS "Setting default install directory to ${CMAKE_INSTALL_PREFIX}")
endif ()

# Generate compilation database
if (CKBB_EXPORT_COMPILE_COMMANDS)
    set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
endif ()

if (NOT VIRTOOLS_SDK_PATH)
    if (DEFINED ENV{VIRTOOLS_SDK_PATH})
        set(VIRTOOLS_SDK_PATH "$ENV{VIRTOOLS_SDK_PATH}" CACHE PATH "Path to the Virtools SDK" FORCE)
    elseif (DEFINED ENV{VIRTOOLS_SDK})
        set(VIRTOOLS_SDK_PATH "$ENV{VIRTOOLS_SDK}" CACHE PATH "Path to the Virtools SDK" FORCE)
    endif ()
endif ()

if (NOT VIRTOOLS_SDK_PATH)
    if (NOT VIRTOOLS_SDK_FETCH_FROM_GIT)
        message(FATAL_ERROR "Virtools SDK location was not specified. Please set VIRTOOLS_SDK_PATH or set VIRTOOLS_SDK_FETCH_FROM_GIT to on to fetch from git.")
    else ()
        include(FetchContent)
        set(FETCHCONTENT_BASE_DIR_SAVE ${FETCHCONTENT_BASE_DIR})
        if (VIRTOOLS_SDK_FETCH_FROM_GIT_PATH)
            get_filename_component(FETCHCONTENT_BASE_DIR "${VIRTOOLS_SDK_FETCH_FROM_GIT_PATH}" REALPATH BASE_DIR "${CMAKE_SOURCE_DIR}")
        endif ()
        FetchContent_Declare(
                Virtools_SDK
                GIT_REPOSITORY https://github.com/doyaGu/Virtools-SDK-2.1.git
                GIT_TAG main
        )
        message(STATUS "Downloading Virtools SDK")
        FetchContent_MakeAvailable(Virtools_SDK)
        set(VIRTOOLS_SDK_PATH "${virtools_sdk_SOURCE_DIR}" CACHE PATH "Path to the Virtools SDK" FORCE)
        set(FETCHCONTENT_BASE_DIR ${FETCHCONTENT_BASE_DIR_SAVE})
    endif ()
endif ()

# Find Virtools SDK
find_package(VirtoolsSDK REQUIRED HINTS "${VIRTOOLS_SDK_PATH}")

# Global compile definitions
add_compile_definitions(
        $<$<C_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
        $<$<C_COMPILER_ID:MSVC>:_CRT_NONSTDC_NO_WARNINGS>
)

# Define all available building block modules
set(CKBB_ALL_MODULES
        3DTrans
        BuildingBlocksAddons1
        Cameras
        Characters
        Collision
        Controllers
        Grids
        Interface
        Lights
        Logics
        Materials-Textures
        MeshModifiers
        MidiManager
        Narratives
        ParticlesSystem
        Sounds
        Visuals
        WorldEnvironment
)

# Create individual options for each module
if (NOT CKBB_BUILD_ALL_MODULES)
    foreach (MODULE IN LISTS CKBB_ALL_MODULES)
        option(CKBB_BUILD_${MODULE} "Build ${MODULE} module" OFF)
    endforeach ()
endif ()

# Add subdirectories for enabled modules
foreach (MODULE IN LISTS CKBB_ALL_MODULES)
    set(BUILD_THIS_MODULE OFF)

    if (CKBB_BUILD_ALL_MODULES)
        set(BUILD_THIS_MODULE ON)
    elseif (CKBB_BUILD_${MODULE})
        set(BUILD_THIS_MODULE ON)
    endif ()

    if (BUILD_THIS_MODULE AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${MODULE}/CMakeLists.txt")
        add_subdirectory(${MODULE})
    endif ()
endforeach ()

# Display configuration summary (only for main project)
if (CKBB_IS_MAIN_PROJECT)
    message(STATUS "")
    message(STATUS "CKBuildingBlocks Configuration Summary:")
    message(STATUS "  Version:              ${PROJECT_VERSION}")
    message(STATUS "  Build Type:           ${CMAKE_BUILD_TYPE}")
    message(STATUS "  Install Prefix:       ${CMAKE_INSTALL_PREFIX}")
    message(STATUS "  Virtools SDK Path:    ${VIRTOOLS_SDK_PATH}")
    message(STATUS "  Build All Modules:    ${CKBB_BUILD_ALL_MODULES}")
    message(STATUS "")
endif ()