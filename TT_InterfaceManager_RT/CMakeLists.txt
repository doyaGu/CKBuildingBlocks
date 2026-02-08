# TT_InterfaceManager_RT

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/Behaviors BEHAVIORS_SRCS)
ckbb_add_module(TT_InterfaceManager_RT
        SOURCES TT_InterfaceManager_RT.cpp
                GameInfo.h
                NemoArray.h
                NemoArray.cpp
                InterfaceManager.h
                InterfaceManager.cpp
                ${BEHAVIORS_SRCS}
)