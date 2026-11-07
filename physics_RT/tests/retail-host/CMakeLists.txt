cmake_minimum_required(VERSION 3.21)
project(PhysicsRetailValidation LANGUAGES C CXX)

# The ordinary Ballanced target imports the reconstructed CK2/VxMath ABI.
# This separate validation target imports the original 2.1 SDK ABI so the
# same physics sources can be tested in an existing original installation.
if (NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "Original runtime validation requires MSVC Win32")
endif ()
set(PHYSICS_RT_RETAIL_SDK_ROOT "" CACHE PATH "Original Virtools 2.1 SDK (Include and Lib)")
foreach (file Include/CKAll.h Lib/CK2.lib Lib/VxMath.lib)
    if (NOT EXISTS "${PHYSICS_RT_RETAIL_SDK_ROOT}/${file}")
        message(FATAL_ERROR "Set PHYSICS_RT_RETAIL_SDK_ROOT to an original Virtools 2.1 SDK")
    endif ()
endforeach ()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
get_filename_component(physics_root "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
set(IVP_INCLUDE_QHULL ON CACHE BOOL "" FORCE)
set(IVP_REENTRANT_QHULL OFF CACHE BOOL "" FORCE)
set(IVP_BUILD_TESTS OFF CACHE BOOL "Build IVP numerical tests")
add_subdirectory("${physics_root}/ivp" ivp)

get_filename_component(default_vxmath_root "${physics_root}/../../VxMath" ABSOLUTE)
set(VXMATH_ROOT "${default_vxmath_root}" CACHE PATH "VxMath component for scalar arithmetic")
if (NOT EXISTS "${VXMATH_ROOT}/cmake/VxScalar.cmake")
    message(FATAL_ERROR "Set VXMATH_ROOT to the VxMath component")
endif ()
include("${VXMATH_ROOT}/cmake/VxScalar.cmake")
vx_add_scalar_arithmetic(physics_RT_scalar_arithmetic)

aux_source_directory("${physics_root}/Behaviors" behavior_sources)
add_library(physics_RT_retail_validation SHARED
        "${physics_root}/physics_RT.cpp"
        "${physics_root}/CKIpionManager.cpp"
        "${physics_root}/PhysicsCallback.cpp"
        "${physics_root}/PhysicsContact.cpp"
        "${physics_root}/PhysicsState.cpp"
        ${behavior_sources})
target_include_directories(physics_RT_retail_validation PRIVATE
        "${PHYSICS_RT_RETAIL_SDK_ROOT}/Include"
        "${physics_root}")
target_compile_definitions(physics_RT_retail_validation PRIVATE
        NOMINMAX WIN32 _WINDOWS _CRT_SECURE_NO_WARNINGS
        PHYSICS_RT_ENABLE_DIAGNOSTICS=0)
target_link_libraries(physics_RT_retail_validation PRIVATE
        "${PHYSICS_RT_RETAIL_SDK_ROOT}/Lib/CK2.lib"
        "${PHYSICS_RT_RETAIL_SDK_ROOT}/Lib/VxMath.lib"
        ivp_controller ivp_compact_builder ivp_physics ivp_collision physics_RT_scalar_arithmetic)
ivp_enable_numerical_ipo(physics_RT_retail_validation)
set_target_properties(physics_RT_retail_validation PROPERTIES OUTPUT_NAME physics_RT)
