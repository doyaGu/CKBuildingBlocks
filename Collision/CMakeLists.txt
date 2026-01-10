# Collision
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/Behaviors BEHAVIORS_SRCS)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/Managers MANAGERS_SRCS)
ckbb_add_module(Collision
        SOURCES
        CollisionBehaviors.cpp
        CollisionManager.cpp
        CollisionManager.h
        CKCollisionManager_AdvancedIntersectionFunctions.cpp
        CKCollisionManager_Detection.cpp
        CKCollisionManager_ToolsFunctions.cpp
        BoundariesManager2.cpp
        BoundariesManager2.h
        FloorManager.cpp
        FloorManager.h
        FloorTools.cpp
        FloorTools.h
        N3dGraph.cpp
        N3dGraph.h
        CalcCollision.cpp
        CalcCollision.h
        CollisionBehaviors.rc
        ${BEHAVIORS_SRCS}
        ${MANAGERS_SRCS}
        OUTPUT_NAME Collisions
)