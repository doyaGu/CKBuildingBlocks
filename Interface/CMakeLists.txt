# Interface
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/Behaviors BEHAVIORS_SRCS)
ckbb_add_module(Interface
        SOURCES
        CKBitmapFont.cpp
        CKBitmapFont.h
        CKFontManager.cpp
        CKFontManager.h
        CKTextureFont.cpp
        CKTextureFont.h
        CompiledData.cpp
        CompiledData.h
        DrawingPrimitives.cpp
        DrawingPrimitives.h
        Layout.h
        Interface.cpp
        Interface.rc
        ${BEHAVIORS_SRCS}
)

if (NOT WIN32)
    if (TARGET Interface)
        vx_add_font_search(Interface)
        target_link_libraries(Interface PRIVATE stb)
    endif ()
    if (TARGET InterfaceStatic)
        vx_add_font_search(InterfaceStatic)
        target_link_libraries(InterfaceStatic PRIVATE stb)
    endif ()
endif ()

ckbb_tests_enabled(Interface _module_tests_enabled)
if (_module_tests_enabled AND NOT WIN32)
    include(CTest)
    enable_testing()
    add_executable(test_interface_stb_font_backend
            tests/test_stb_font_backend.cpp
    )
    vx_add_font_search(test_interface_stb_font_backend)
    target_include_directories(test_interface_stb_font_backend PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(test_interface_stb_font_backend PRIVATE stb VxMath)
    target_compile_features(test_interface_stb_font_backend PRIVATE cxx_std_17)
    target_compile_definitions(test_interface_stb_font_backend PRIVATE
            TEST_FONT_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/fonts"
    )
    add_test(NAME test_interface_stb_font_backend COMMAND test_interface_stb_font_backend)
    if (TARGET Interface)
        add_executable(test_interface_font_manager tests/test_font_manager.cpp)
        vx_add_font_search(test_interface_font_manager)
        target_include_directories(test_interface_font_manager PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
        target_link_libraries(test_interface_font_manager PRIVATE Interface CK2 VxMath)
        target_compile_features(test_interface_font_manager PRIVATE cxx_std_17)
        vx_prepare_font_assets(_font_directory _font_files)
        add_custom_command(TARGET test_interface_font_manager POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:test_interface_font_manager>/data/fonts"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${_font_files} "$<TARGET_FILE_DIR:test_interface_font_manager>/data/fonts"
                COMMAND_EXPAND_LISTS VERBATIM)
        add_test(NAME test_interface_font_manager COMMAND test_interface_font_manager)
    endif ()
    if (TARGET CK2_3DStatic AND TARGET InterfaceStatic)
        vx_prepare_font_assets(_font_directory _font_files)
        add_executable(test_interface_font_atlas tests/test_font_atlas.cpp)
        vx_add_font_search(test_interface_font_atlas)
        target_include_directories(test_interface_font_atlas PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
        target_link_libraries(test_interface_font_atlas PRIVATE InterfaceStatic CK2_3DStatic)
        target_compile_features(test_interface_font_atlas PRIVATE cxx_std_17)
        target_compile_definitions(test_interface_font_atlas PRIVATE
                TEST_FONT_FILE="${_font_directory}/NotoSans-Bold.ttf"
                TEST_REGULAR_FONT_FILE="${_font_directory}/NotoSans-Regular.ttf")
        set_target_properties(test_interface_font_atlas PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/font-atlas")
        add_test(NAME test_interface_font_atlas COMMAND test_interface_font_atlas)
        add_test(NAME test_interface_font_atlas_italic COMMAND test_interface_font_atlas --italic)
        add_test(NAME test_interface_font_atlas_clip COMMAND test_interface_font_atlas --clip)
        add_test(NAME test_interface_font_atlas_native COMMAND test_interface_font_atlas --native)
        set_tests_properties(test_interface_font_atlas test_interface_font_atlas_italic test_interface_font_atlas_clip test_interface_font_atlas_native PROPERTIES
                RESOURCE_LOCK InterfaceFontAtlas)
    endif ()
endif ()
