# Lights
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/Behaviors BEHAVIORS_SRCS)
ckbb_add_module(Lights SOURCES Lights.cpp Lights.rc ${BEHAVIORS_SRCS})
