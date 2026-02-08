# TT_Toolbox_RT

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/Behaviors BEHAVIORS_SRCS)
ckbb_add_module(TT_Toolbox_RT
        SOURCES TT_Toolbox_RT.cpp
                ToolboxGuids.h
                TimeManager.h
                TimeManager.cpp
                DebugManager.h
                DebugManager.cpp
                DXManager.h
                DXManager.cpp
                ${BEHAVIORS_SRCS}
)