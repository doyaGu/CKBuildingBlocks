# Logics
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/Behaviors BEHAVIORS_SRCS)
ckbb_add_module(Logics SOURCES Logics.cpp Logics.rc ${BEHAVIORS_SRCS})

ckbb_tests_enabled(Logics _module_tests_enabled)
if (_module_tests_enabled)
    add_executable(LogicsRandomContextTest tests/RandomContextTest.cpp
            Behaviors/perlin.cpp Behaviors/Random.cpp)
    target_link_libraries(LogicsRandomContextTest PRIVATE CK2 VxMath)
    target_compile_features(LogicsRandomContextTest PRIVATE cxx_std_17)
    set_target_properties(LogicsRandomContextTest PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
    if (WIN32)
        foreach (_runtime IN ITEMS CK2 VxMath SDL3::SDL3)
            if (TARGET ${_runtime})
                get_target_property(_type ${_runtime} TYPE)
                if (_type STREQUAL "SHARED_LIBRARY")
                    add_custom_command(TARGET LogicsRandomContextTest POST_BUILD
                            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                            "$<TARGET_FILE:${_runtime}>" "$<TARGET_FILE_DIR:LogicsRandomContextTest>"
                            VERBATIM)
                endif ()
            endif ()
        endforeach ()
    endif ()
    add_test(NAME LogicsRandomContextTest COMMAND LogicsRandomContextTest)
endif ()
