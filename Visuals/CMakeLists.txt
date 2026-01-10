# Visuals
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/Behaviors BEHAVIORS_SRCS)
ckbb_add_module(Visuals SOURCES Visuals.cpp Visuals.rc ${BEHAVIORS_SRCS})