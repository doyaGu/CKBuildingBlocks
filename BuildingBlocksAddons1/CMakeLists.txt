# BuildingBlocksAddons1
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/Behaviors BEHAVIORS_SRCS)
ckbb_add_module(BuildingBlocksAddons1
        SOURCES
        BuildingBlocksAddons1.cpp
        LightmapGenerator.cpp
        LightmapGenerator.h
        NearestPointGrid.cpp
        NearestPointGrid.h
        PolyhedronConstructor.cpp
        PolyhedronConstructor.h
        TextureProcessing.cpp
        TextureProcessing.h
        BuildingBlocksAddons.rc
        ${BEHAVIORS_SRCS}
)

if (CKBB_BUILD_TESTS AND BUILD_TESTING AND TARGET CK2_3DStatic)
    add_executable(BuildingBlocksAddons1TextureProcessingTest
            tests/TextureProcessingTest.cpp
            TextureProcessing.cpp
    )
    target_compile_features(BuildingBlocksAddons1TextureProcessingTest PRIVATE cxx_std_17)
    target_link_libraries(BuildingBlocksAddons1TextureProcessingTest PRIVATE CK2_3DStatic)
    if (COMMAND ckre_copy_runtime_dependencies)
        ckre_copy_runtime_dependencies(BuildingBlocksAddons1TextureProcessingTest)
    endif ()
    add_test(NAME BuildingBlocksAddons1TextureProcessingTest COMMAND BuildingBlocksAddons1TextureProcessingTest)
    set_tests_properties(BuildingBlocksAddons1TextureProcessingTest PROPERTIES TIMEOUT 60)
endif ()
