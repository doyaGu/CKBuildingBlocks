# MeshModifiers
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/Behaviors BEHAVIORS_SRCS)
ckbb_add_module(MeshModifiers SOURCES MeshModifiers.cpp MeshModifiers.rc ${BEHAVIORS_SRCS})

ckbb_tests_enabled(MeshModifiers _module_tests_enabled)
if (_module_tests_enabled)
    add_executable(MeshNoiseContextTest tests/NoiseContextTest.cpp
            Behaviors/Noise.cpp Behaviors/MeshModificationsCallback.cpp)
    target_link_libraries(MeshNoiseContextTest PRIVATE CK2 VxMath)
    target_compile_features(MeshNoiseContextTest PRIVATE cxx_std_17)
    set_target_properties(MeshNoiseContextTest PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    if (WIN32)
        foreach (_runtime IN ITEMS CK2 VxMath SDL3::SDL3)
            if (TARGET ${_runtime})
                get_target_property(_type ${_runtime} TYPE)
                if (_type STREQUAL "SHARED_LIBRARY")
                    add_custom_command(TARGET MeshNoiseContextTest POST_BUILD
                            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                            "$<TARGET_FILE:${_runtime}>" "$<TARGET_FILE_DIR:MeshNoiseContextTest>"
                            VERBATIM)
                endif ()
            endif ()
        endforeach ()
    endif ()
    add_test(NAME MeshNoiseContextTest COMMAND MeshNoiseContextTest)
endif ()
