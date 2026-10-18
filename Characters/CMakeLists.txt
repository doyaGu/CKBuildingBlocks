# Characters
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/Behaviors BEHAVIORS_SRCS)
ckbb_add_module(Characters SOURCES Characters.cpp Characters.rc ${BEHAVIORS_SRCS})

if (CKBB_BUILD_TESTS AND BUILD_TESTING AND TARGET CK2_3DStatic)
    add_executable(UnlimitedControllerTest
            tests/UnlimitedControllerTest.cpp
            Behaviors/UnlimitedController.cpp
    )
    target_compile_features(UnlimitedControllerTest PRIVATE cxx_std_17)
    target_link_libraries(UnlimitedControllerTest PRIVATE CK2_3DStatic)
    add_test(NAME UnlimitedControllerTest COMMAND UnlimitedControllerTest)
    set_tests_properties(UnlimitedControllerTest PROPERTIES
            WORKING_DIRECTORY "$<TARGET_FILE_DIR:CK2>"
    )
endif ()
