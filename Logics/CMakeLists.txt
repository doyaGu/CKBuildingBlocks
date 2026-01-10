# Logics
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/Behaviors BEHAVIORS_SRCS)
ckbb_add_module(Logics SOURCES Logics.cpp Logics.rc ${BEHAVIORS_SRCS})
