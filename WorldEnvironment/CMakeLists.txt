# WorldEnvironment
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/Behaviors BEHAVIORS_SRCS)
ckbb_add_module(WorldEnvironment SOURCES WorldEnvironment.cpp WorldEnvironment.rc ${BEHAVIORS_SRCS})