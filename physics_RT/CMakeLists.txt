# Match the original hull ordering, including in previously configured builds.
set(IVP_INCLUDE_QHULL ON CACHE BOOL "Include the original convex hull builder" FORCE)
set(IVP_REENTRANT_QHULL OFF CACHE BOOL "Use the original Ipion hull builder" FORCE)
option(IVP_ENABLE_X87_EMULATION "Emulate original extended arithmetic" ON)
option(PHYSICS_RT_ENABLE_DIAGNOSTICS "Export internal Win32 physics diagnostics" OFF)
option(PHYSICS_RT_BUILD_PARITY_TESTS "Build Win32 original-image differential tests" OFF)
if (PHYSICS_RT_ENABLE_DIAGNOSTICS AND
        (NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 4))
    message(FATAL_ERROR "physics_RT internal diagnostics require MSVC Win32")
endif ()
if (PHYSICS_RT_BUILD_PARITY_TESTS AND NOT PHYSICS_RT_ENABLE_DIAGNOSTICS)
    message(FATAL_ERROR "physics_RT differential tests require PHYSICS_RT_ENABLE_DIAGNOSTICS=ON")
endif ()

add_subdirectory(ivp)

set(PHYSICS_RT_IVP_LIBRARIES ivp_controller ivp_compact_builder ivp_physics ivp_collision)
if (CMAKE_LINK_GROUP_USING_RESCAN_SUPPORTED)
    set(PHYSICS_RT_IVP_LIBRARIES
            "$<LINK_GROUP:RESCAN,ivp_controller,ivp_compact_builder,ivp_physics,ivp_surface_manager,ivp_utility,ivp_collision>"
    )
endif ()

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/Behaviors BEHAVIORS_SRCS)
set(PHYSICS_RT_SOURCES physics_RT.cpp
                CKIpionManager.cpp
                CKIpionManager.h
                PhysicsCallback.cpp
                PhysicsCallback.h
                PhysicsContact.cpp
                PhysicsContact.h
                PhysicsState.cpp
                PhysicsState.h
                PhysicsStateClock.h
                Convert.h
                PhysicsSDK.h
                ${BEHAVIORS_SRCS}
)
if (PHYSICS_RT_ENABLE_DIAGNOSTICS)
    list(APPEND PHYSICS_RT_SOURCES
            Diagnostics/PhysicsCoreDiagnostics.cpp
            Diagnostics/PhysicsContactDiagnostics.cpp
            Diagnostics/PhysicsSolverDiagnostics.cpp)
endif ()
ckbb_add_module(physics_RT SOURCES ${PHYSICS_RT_SOURCES} LINKS ${PHYSICS_RT_IVP_LIBRARIES})

function(physics_rt_configure_host target)
    # IVP's Win32 precision contract propagates /arch:IA32. Use VxMath's
    # scalar inline operations here; its separately built library keeps SIMD.
    if (MSVC AND CMAKE_SIZEOF_VOID_P EQUAL 4 AND IVP_ENABLE_X87_EMULATION)
        target_compile_definitions(${target} PRIVATE VX_SIMD_NONE=1)
    endif ()
endfunction()

foreach (target physics_RT physics_RTStatic)
    if (TARGET ${target})
        physics_rt_configure_host(${target})
        target_compile_definitions(${target} PRIVATE
                PHYSICS_RT_STABLE_ENTITY_ORDER=1
                PHYSICS_RT_ENABLE_DIAGNOSTICS=$<BOOL:${PHYSICS_RT_ENABLE_DIAGNOSTICS}>)
        if (IVP_ENABLE_X87_EMULATION)
            ivp_enable_numerical_ipo(${target})
        endif ()
    endif ()
endforeach ()

if (CKBB_BUILD_TESTS AND BUILD_TESTING AND TARGET CK2_3DStatic)
    add_executable(physics_rt_regression_tests
            tests/test_physics_regressions.cpp
            tests/PhysicsHostTest.cpp
    )
    target_compile_features(physics_rt_regression_tests PRIVATE cxx_std_17)
    physics_rt_configure_host(physics_rt_regression_tests)
    target_compile_definitions(physics_rt_regression_tests PRIVATE
            CKBB_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
    )
    target_include_directories(physics_rt_regression_tests PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}"
    )
    target_link_libraries(physics_rt_regression_tests PRIVATE
            CK2_3DStatic
            ${PHYSICS_RT_IVP_LIBRARIES}
    )
    if (TARGET physics_RTStatic)
        target_link_libraries(physics_rt_regression_tests PRIVATE physics_RTStatic)
    else ()
        target_sources(physics_rt_regression_tests PRIVATE ${PHYSICS_RT_SOURCES})
        target_compile_definitions(physics_rt_regression_tests PRIVATE
                PHYSICS_RT_ENABLE_DIAGNOSTICS=$<BOOL:${PHYSICS_RT_ENABLE_DIAGNOSTICS}>)
    endif ()
    set_target_properties(physics_rt_regression_tests PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
    add_test(NAME physics_rt_regression_tests
            COMMAND $<TARGET_FILE:physics_rt_regression_tests>)
    if (WIN32)
        add_custom_command(TARGET physics_rt_regression_tests POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:physics_rt_regression_tests>
                $<TARGET_FILE_DIR:physics_rt_regression_tests>
                COMMAND_EXPAND_LISTS)
    endif ()
    set_tests_properties(physics_rt_regression_tests PROPERTIES TIMEOUT 60 LABELS "physics;host")
elseif (CKBB_BUILD_TESTS AND BUILD_TESTING)
    message(STATUS "physics_RT host tests need CKRE_BUILD_STATIC=ON (CK2_3DStatic)")
endif ()

if (CKBB_BUILD_TESTS AND BUILD_TESTING AND WIN32 AND TARGET physics_RT)
    add_executable(physics_rt_module_tests tests/PhysicsModuleTest.cpp)
    target_include_directories(physics_rt_module_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    add_dependencies(physics_rt_module_tests physics_RT)
    set_target_properties(physics_rt_module_tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    add_custom_command(TARGET physics_rt_module_tests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:physics_RT> $<TARGET_FILE_DIR:physics_rt_module_tests>
            COMMAND_EXPAND_LISTS)
    add_test(NAME physics_rt_module_tests COMMAND physics_rt_module_tests
            $<TARGET_FILE:physics_RT> $<BOOL:${PHYSICS_RT_ENABLE_DIAGNOSTICS}>)
    set_tests_properties(physics_rt_module_tests PROPERTIES TIMEOUT 30 LABELS "physics;module")
endif ()

if (IVP_BUILD_TESTS AND IVP_ENABLE_X87_EMULATION)
    add_subdirectory(ivp/tests/numerics ivp-numerics-tests)
endif ()
if (PHYSICS_RT_BUILD_PARITY_TESTS)
    add_subdirectory(tests)
endif ()
