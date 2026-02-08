# TT_Gravity_RT

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/Behaviors BEHAVIORS_SRCS)
ckbb_add_module(TT_Gravity_RT
        SOURCES TT_Gravity_RT.cpp ${BEHAVIORS_SRCS}
)