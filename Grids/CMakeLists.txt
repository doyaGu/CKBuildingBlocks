# Grids

add_library(Grids SHARED
        Grids.cpp
        CharacterGridPathFollow.cpp
        ComputePath.h
        ContextInfo.cpp
        ContextInfo.h
        FollowProblem.cpp
        FollowProblem.h
        GridInfo.cpp
        GridInfo.h
        GridManager.cpp
        GridManager.h
        GridPathInit.cpp
        GridPathManager.cpp
        GridPathManager.h
        GridPathManagerFollow.cpp
        GridPathSolver.cpp
        LinkerGraph.h
        Node.h
        NodeGrid.cpp
        NodeGrid.h
        NodeLinker.cpp
        NodeLinker.h
        OpenList.cpp
        OpenList.h
        Path.cpp
        Path.h
        Pathfinding.h
        PathProblem.cpp
        PathProblem.h
        PFUtils.h
        Sliceable.h)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/Behaviors GRIDS_BEHAVIORS_SRCS)
target_sources(Grids PRIVATE ${GRIDS_BEHAVIORS_SRCS})
target_include_directories(Grids PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Grids PRIVATE CK2 VxMath)

install(TARGETS Grids RUNTIME DESTINATION BuildingBlocks)
