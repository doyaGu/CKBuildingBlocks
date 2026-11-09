# TT_InterfaceManager_RT

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/Behaviors BEHAVIORS_SRCS)

ckbb_add_module(TT_InterfaceManager_RT
        SOURCES TT_InterfaceManager_RT.cpp
                GameInfo.h
                NemoArray.h
                NemoArray.cpp
                InterfaceManager.h
                InterfaceManager.cpp
                RegistryUtils.h
                RegistryUtils.cpp
                ${BEHAVIORS_SRCS}
)

if (CKBB_BUILD_TESTS AND BUILD_TESTING AND TARGET CK2_3DStatic)
    add_executable(TTInterfaceManagerRetailTest
            tests/RetailBehaviorTest.cpp
            Behaviors/ReadRegistry.cpp
            NemoArray.cpp
            RegistryUtils.cpp
    )
    target_compile_features(TTInterfaceManagerRetailTest PRIVATE cxx_std_17)
    target_include_directories(TTInterfaceManagerRetailTest PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(TTInterfaceManagerRetailTest PRIVATE CK2_3DStatic)
    if (COMMAND ckre_copy_runtime_dependencies)
        ckre_copy_runtime_dependencies(TTInterfaceManagerRetailTest)
    endif ()
    add_test(NAME TTInterfaceManagerRetailTest COMMAND TTInterfaceManagerRetailTest)
    set_tests_properties(TTInterfaceManagerRetailTest PROPERTIES
            TIMEOUT 60
            WORKING_DIRECTORY "$<TARGET_FILE_DIR:CK2>"
    )
endif ()
