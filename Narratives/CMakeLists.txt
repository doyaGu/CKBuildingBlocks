# Narratives
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/Behaviors BEHAVIORS_SRCS)
ckbb_add_module(Narratives SOURCES Narratives.cpp Narratives.rc ${BEHAVIORS_SRCS})