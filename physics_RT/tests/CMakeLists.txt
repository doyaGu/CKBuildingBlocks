# Keep the reference DLL's CK2/VxMath separate from the statically linked host.
if (NOT TARGET physics_RTStatic)
    message(FATAL_ERROR "physics_RT differential tests require CKBB_BUILD_STATIC=ON")
endif ()
set(PHYSICS_RT_RETAIL_DLL "" CACHE FILEPATH "Unmodified reference physics DLL")
set(PHYSICS_RT_RETAIL_BIN "" CACHE PATH "Reference runtime dependencies")
add_executable(physics_rt_parity
        retail_parity.cpp
        RetailMathTests.cpp
        RetailGeometryTests.cpp
        RetailDynamicsTests.cpp
        RetailContactTests.cpp
        RetailPressureTests.cpp
        RetailContinuousTests.cpp
        RetailCapturedContinuous.cpp
)
target_compile_features(physics_rt_parity PRIVATE cxx_std_17)
target_link_libraries(physics_rt_parity PRIVATE ivp_controller ivp_compact_builder ivp_physics ivp_collision physics_RTStatic advapi32)
add_custom_command(TARGET physics_rt_parity POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_RUNTIME_DLLS:physics_rt_parity> $<TARGET_FILE_DIR:physics_rt_parity>
        COMMAND_EXPAND_LISTS)
if (PHYSICS_RT_RETAIL_DLL AND PHYSICS_RT_RETAIL_BIN)
    set(registry "${CMAKE_CURRENT_SOURCE_DIR}/retail_scenarios.inc")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${registry}")
    file(STRINGS "${registry}" scenarios REGEX "^RETAIL_SCENARIO\\(")
    foreach (entry IN LISTS scenarios)
        string(REGEX MATCH "^RETAIL_SCENARIO\\(([a-z0-9_]+), ([a-z_]+)," matched "${entry}")
        if (NOT matched)
            message(FATAL_ERROR "Invalid retail scenario: ${entry}")
        endif ()
        set(scenario "${CMAKE_MATCH_1}")
        set(family "${CMAKE_MATCH_2}")
        add_test(NAME retail_${scenario} COMMAND physics_rt_parity
            "${PHYSICS_RT_RETAIL_DLL}" "${PHYSICS_RT_RETAIL_BIN}" ${scenario})
        set_tests_properties(retail_${scenario} PROPERTIES TIMEOUT 30 LABELS "retail;${family}")
    endforeach ()
endif ()
