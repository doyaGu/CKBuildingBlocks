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