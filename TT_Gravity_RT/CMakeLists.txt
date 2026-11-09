# TT_Gravity_RT

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/Behaviors BEHAVIORS_SRCS)
ckbb_add_module(TT_Gravity_RT
        SOURCES TT_Gravity_RT.cpp ${BEHAVIORS_SRCS}
)

if (CKBB_BUILD_TESTS AND BUILD_TESTING AND TARGET CK2_3DStatic)
    add_executable(RealShadowMappingGeometryTest tests/RealShadowMappingGeometryTest.cpp)
    target_compile_features(RealShadowMappingGeometryTest PRIVATE cxx_std_17)
    target_link_libraries(RealShadowMappingGeometryTest PRIVATE CK2_3DStatic)
    if (COMMAND ckre_copy_runtime_dependencies)
        ckre_copy_runtime_dependencies(RealShadowMappingGeometryTest)
    endif ()
    add_test(NAME RealShadowMappingGeometryTest COMMAND RealShadowMappingGeometryTest)
    set_tests_properties(RealShadowMappingGeometryTest PROPERTIES
            TIMEOUT 60
            WORKING_DIRECTORY "$<TARGET_FILE_DIR:CK2>"
    )
endif ()
