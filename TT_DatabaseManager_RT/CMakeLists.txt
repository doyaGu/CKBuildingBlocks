# TT_DatabaseManager_RT

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/Behaviors BEHAVIORS_SRCS)
ckbb_add_module(TT_DatabaseManager_RT
        SOURCES TT_DatabaseManager_RT.cpp
                DatabaseManager.cpp
                DatabaseManager.h
                ${BEHAVIORS_SRCS}
)