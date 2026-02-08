# =============================================================================
# CKBuildingBlocks - Virtools Building Blocks Collection
# =============================================================================
cmake_minimum_required(VERSION 3.16)

project(CKBuildingBlocks
        VERSION 2.1.0.14
        DESCRIPTION "Virtools Building Blocks Collection"
        LANGUAGES C CXX
)

# =============================================================================
# Determine if top-level project
# =============================================================================
if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.21")
    if (PROJECT_IS_TOP_LEVEL)
        set(CKBB_IS_TOP_LEVEL ON)
    else ()
        set(CKBB_IS_TOP_LEVEL OFF)
    endif ()
else ()
    if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
        set(CKBB_IS_TOP_LEVEL ON)
    else ()
        set(CKBB_IS_TOP_LEVEL OFF)
    endif ()
endif ()

# =============================================================================
# Platform and build checks
# =============================================================================
if (CKBB_IS_TOP_LEVEL AND CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "In-source builds not allowed. Run CMake from a separate directory: cmake -B build")
endif ()

if (NOT WIN32)
    message(FATAL_ERROR "CKBuildingBlocks only supports Windows.")
endif ()

# =============================================================================
# C++ standard
# =============================================================================
if (NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 98)
endif ()
if (NOT DEFINED CMAKE_CXX_STANDARD_REQUIRED)
    set(CMAKE_CXX_STANDARD_REQUIRED NO)
endif ()
if (NOT DEFINED CMAKE_CXX_EXTENSIONS)
    set(CMAKE_CXX_EXTENSIONS YES)
endif ()

# =============================================================================
# Project options
# =============================================================================
option(CKBB_BUILD_ALL_MODULES "Build all available building block modules" ON)
option(CKBB_BUILD_SHARED "Build building blocks as DLLs (SHARED)" ON)
option(CKBB_BUILD_STATIC "Build building blocks as static libraries (STATIC)" OFF)
option(CKBB_INSTALL "Generate install target" ${CKBB_IS_TOP_LEVEL})

# =============================================================================
# CMake modules
# =============================================================================
include(GNUInstallDirs)

# =============================================================================
# Top-level project settings
# =============================================================================
if (CKBB_IS_TOP_LEVEL)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
    set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMakeTargets")
    set(CMAKE_USE_RELATIVE_PATHS TRUE)
    set(CMAKE_SUPPRESS_REGENERATION TRUE)

    get_property(IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
    if (NOT IS_MULTI_CONFIG AND NOT CMAKE_BUILD_TYPE)
        message(STATUS "[CKBuildingBlocks] Setting build type to 'Release'")
        set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the build type" FORCE)
        set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
    endif ()

    if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        set(CMAKE_INSTALL_PREFIX "${PROJECT_SOURCE_DIR}/install" CACHE PATH "Installation directory" FORCE)
        message(STATUS "[CKBuildingBlocks] Setting install directory to ${CMAKE_INSTALL_PREFIX}")
    endif ()

    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif ()

# =============================================================================
# Compiler settings
# =============================================================================
if (MSVC)
    add_compile_definitions(
            _CRT_SECURE_NO_WARNINGS
            _CRT_NONSTDC_NO_WARNINGS
    )
endif ()

# =============================================================================
# Virtools SDK
# =============================================================================
if (NOT TARGET VxMath OR NOT TARGET CK2)
    set(VIRTOOLS_SDK_PATH "" CACHE PATH "Path to the Virtools SDK")
    option(VIRTOOLS_SDK_FETCH_FROM_GIT "Fetch Virtools SDK from git if not found" OFF)
    set(VIRTOOLS_SDK_FETCH_FROM_GIT_PATH "" CACHE FILEPATH "Location to download SDK")

    if (NOT VIRTOOLS_SDK_PATH)
        if (DEFINED ENV{VIRTOOLS_SDK_PATH})
            set(VIRTOOLS_SDK_PATH "$ENV{VIRTOOLS_SDK_PATH}" CACHE PATH "Path to the Virtools SDK" FORCE)
        elseif (DEFINED ENV{VIRTOOLS_SDK})
            set(VIRTOOLS_SDK_PATH "$ENV{VIRTOOLS_SDK}" CACHE PATH "Path to the Virtools SDK" FORCE)
        endif ()
    endif ()

    if (NOT VIRTOOLS_SDK_PATH)
        if (VIRTOOLS_SDK_FETCH_FROM_GIT)
            include(FetchContent)
            set(FETCHCONTENT_BASE_DIR_SAVE ${FETCHCONTENT_BASE_DIR})
            if (VIRTOOLS_SDK_FETCH_FROM_GIT_PATH)
                get_filename_component(FETCHCONTENT_BASE_DIR "${VIRTOOLS_SDK_FETCH_FROM_GIT_PATH}" REALPATH BASE_DIR "${CMAKE_SOURCE_DIR}")
            endif ()
            FetchContent_Declare(
                    virtools_sdk
                    GIT_REPOSITORY https://github.com/doyaGu/Virtools-SDK-2.1.git
                    GIT_TAG main
            )
            message(STATUS "[CKBuildingBlocks] Downloading Virtools SDK")
            FetchContent_MakeAvailable(virtools_sdk)
            set(VIRTOOLS_SDK_PATH "${virtools_sdk_SOURCE_DIR}" CACHE PATH "Path to the Virtools SDK" FORCE)
            set(FETCHCONTENT_BASE_DIR ${FETCHCONTENT_BASE_DIR_SAVE})
        else ()
            message(FATAL_ERROR "Virtools SDK location was not specified. Please set VIRTOOLS_SDK_PATH or enable VIRTOOLS_SDK_FETCH_FROM_GIT.")
        endif ()
    endif ()

    find_path(VIRTOOLS_SDK_INCLUDE_DIR CKAll.h
            HINTS $ENV{VIRTOOLS_SDK_PATH} ${VIRTOOLS_SDK_PATH}
            PATH_SUFFIXES Include Includes include includes
    )
    if (NOT VIRTOOLS_SDK_INCLUDE_DIR)
        message(FATAL_ERROR "Virtools SDK headers not found. Expected CKAll.h under ${VIRTOOLS_SDK_PATH}.")
    endif ()

    foreach (_lib IN ITEMS CK2 VxMath)
        find_library(VIRTOOLS_SDK_${_lib}_LIB NAMES ${_lib}
                HINTS $ENV{VIRTOOLS_SDK_PATH} ${VIRTOOLS_SDK_PATH}
                PATH_SUFFIXES Lib lib
        )
        if (NOT VIRTOOLS_SDK_${_lib}_LIB)
            message(FATAL_ERROR "Virtools SDK library '${_lib}' not found under ${VIRTOOLS_SDK_PATH}.")
        endif ()

        if (NOT TARGET ${_lib})
            add_library(${_lib} UNKNOWN IMPORTED)
            set_target_properties(${_lib} PROPERTIES
                    IMPORTED_LOCATION "${VIRTOOLS_SDK_${_lib}_LIB}"
                    INTERFACE_INCLUDE_DIRECTORIES "${VIRTOOLS_SDK_INCLUDE_DIR}"
            )
            target_compile_options(${_lib} INTERFACE
                    $<$<COMPILE_LANGUAGE:CXX>:$<$<CXX_COMPILER_ID:MSVC>:/Zc:strictStrings->>
                    $<$<COMPILE_LANGUAGE:CXX>:$<$<CXX_COMPILER_ID:GNU>:-fpermissive>>
                    $<$<COMPILE_LANGUAGE:CXX>:$<$<CXX_COMPILER_ID:GNU>:-Wno-write-strings>>
            )
        endif ()
    endforeach ()
endif ()

# =============================================================================
# Define all available modules
# =============================================================================
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
        Sounds
        Visuals
        WorldEnvironment

        # Custom building blocks
        physics_RT
        TT_DatabaseManager_RT
        TT_Gravity_RT
        TT_InterfaceManager_RT
        TT_ParticleSystems_RT
        TT_Toolbox_RT
)

# Create individual options for each module
if (NOT CKBB_BUILD_ALL_MODULES)
    foreach (MODULE IN LISTS CKBB_ALL_MODULES)
        option(CKBB_BUILD_${MODULE} "Build ${MODULE} module" OFF)
    endforeach ()
endif ()

# =============================================================================
# Helper function for creating building block modules
# =============================================================================
function(ckbb_add_module TARGET_NAME)
    cmake_parse_arguments(MODULE "" "OUTPUT_NAME" "SOURCES;LINKS" ${ARGN})

    if (CKBB_BUILD_SHARED)
        add_library(${TARGET_NAME} SHARED ${MODULE_SOURCES})
        target_include_directories(${TARGET_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
        target_compile_features(${TARGET_NAME} PRIVATE cxx_std_17)
        target_link_libraries(${TARGET_NAME} PRIVATE CK2 VxMath ${MODULE_LINKS})
        set_target_properties(${TARGET_NAME} PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
                LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
                ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        )

        if (MODULE_OUTPUT_NAME)
            set_target_properties(${TARGET_NAME} PROPERTIES OUTPUT_NAME "${MODULE_OUTPUT_NAME}")
        endif ()

        if (CKBB_INSTALL)
            install(TARGETS ${TARGET_NAME}
                    RUNTIME DESTINATION BuildingBlocks
                    LIBRARY DESTINATION BuildingBlocks
            )
        endif ()
    endif ()

    if (CKBB_BUILD_STATIC)
        # Static libraries should not compile Windows resources (.rc).
        set(CKBB_STATIC_SOURCES ${MODULE_SOURCES})
        list(FILTER CKBB_STATIC_SOURCES EXCLUDE REGEX "\\.rc$")

        add_library(${TARGET_NAME}Static STATIC ${CKBB_STATIC_SOURCES})
        target_include_directories(${TARGET_NAME}Static PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
        target_compile_features(${TARGET_NAME}Static PRIVATE cxx_std_17)

        # Prefer static deps if available.
        set(_ck2_dep CK2)
        if (TARGET CK2Static)
            set(_ck2_dep CK2Static)
        endif ()
        set(_vxmath_dep VxMath)
        if (TARGET VxMathStatic)
            set(_vxmath_dep VxMathStatic)
        endif ()

        target_link_libraries(${TARGET_NAME}Static PRIVATE ${_ck2_dep} ${_vxmath_dep} ${MODULE_LINKS})
        set_target_properties(${TARGET_NAME}Static PROPERTIES
                ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
                POSITION_INDEPENDENT_CODE ON
        )

        if (MODULE_OUTPUT_NAME)
            # Avoid name collision with the DLL import library.
            set_target_properties(${TARGET_NAME}Static PROPERTIES OUTPUT_NAME "${MODULE_OUTPUT_NAME}Static")
        endif ()

        if (CKBB_INSTALL)
            install(TARGETS ${TARGET_NAME}Static
                    ARCHIVE DESTINATION lib
            )
        endif ()
    endif ()
endfunction()

# =============================================================================
# Add subdirectories for enabled modules
# =============================================================================
set(CKBB_BUILT_MODULES "")

foreach (MODULE IN LISTS CKBB_ALL_MODULES)
    set(BUILD_THIS_MODULE OFF)

    if (CKBB_BUILD_ALL_MODULES)
        set(BUILD_THIS_MODULE ON)
    elseif (CKBB_BUILD_${MODULE})
        set(BUILD_THIS_MODULE ON)
    endif ()

    if (BUILD_THIS_MODULE AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${MODULE}/CMakeLists.txt")
        add_subdirectory(${MODULE})
        list(APPEND CKBB_BUILT_MODULES ${MODULE})
    endif ()
endforeach ()

# =============================================================================
# Configuration summary
# =============================================================================
if (CKBB_IS_TOP_LEVEL)
    message(STATUS "")
    message(STATUS "============================================================")
    message(STATUS "CKBuildingBlocks Configuration Summary")
    message(STATUS "============================================================")
    message(STATUS "  Version:              ${PROJECT_VERSION}")
    message(STATUS "  Build Type:           ${CMAKE_BUILD_TYPE}")
    if (VIRTOOLS_SDK_PATH)
        message(STATUS "  Virtools SDK:         ${VIRTOOLS_SDK_PATH}")
    endif ()
    message(STATUS "  Build All Modules:    ${CKBB_BUILD_ALL_MODULES}")
    message(STATUS "  Build Shared:         ${CKBB_BUILD_SHARED}")
    message(STATUS "  Build Static:         ${CKBB_BUILD_STATIC}")
    message(STATUS "  Built Modules:        ${CKBB_BUILT_MODULES}")
    message(STATUS "  Install:              ${CKBB_INSTALL}")
    message(STATUS "  Install Prefix:       ${CMAKE_INSTALL_PREFIX}")
    message(STATUS "============================================================")
    message(STATUS "")
endif ()
