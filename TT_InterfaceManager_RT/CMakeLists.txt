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

ckbb_tests_enabled(TT_InterfaceManager_RT _module_tests_enabled)
if (_module_tests_enabled AND TARGET CK2Static)
    add_executable(TTInterfaceManagerRetailTest
            tests/RetailBehaviorTest.cpp
            Behaviors/ReadRegistry.cpp
            Behaviors/CreateNewGameInfo.cpp
            InterfaceManager.cpp
            NemoArray.cpp
            RegistryUtils.cpp
    )
    target_compile_features(TTInterfaceManagerRetailTest PRIVATE cxx_std_17)
    target_include_directories(TTInterfaceManagerRetailTest PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(TTInterfaceManagerRetailTest PRIVATE CK2Static)
    if (WIN32)
        set(_test_runtimes SDL3::SDL3)
        if (NOT TARGET VxMathStatic)
            list(APPEND _test_runtimes VxMath)
        endif ()
        foreach (_runtime IN LISTS _test_runtimes)
            if (TARGET ${_runtime})
                get_target_property(_type ${_runtime} TYPE)
                if (_type STREQUAL "SHARED_LIBRARY")
                    add_custom_command(TARGET TTInterfaceManagerRetailTest POST_BUILD
                            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                            "$<TARGET_FILE:${_runtime}>" "$<TARGET_FILE_DIR:TTInterfaceManagerRetailTest>"
                            VERBATIM)
                endif ()
            endif ()
        endforeach ()
    endif ()
    add_test(NAME TTInterfaceManagerRetailTest COMMAND TTInterfaceManagerRetailTest)
    set_tests_properties(TTInterfaceManagerRetailTest PROPERTIES
            TIMEOUT 60
            WORKING_DIRECTORY "$<TARGET_FILE_DIR:CK2>"
    )
endif ()
