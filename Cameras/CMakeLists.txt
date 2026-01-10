# Cameras
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/Behaviors BEHAVIORS_SRCS)
ckbb_add_module(Cameras SOURCES Cameras.cpp Cameras.rc ${BEHAVIORS_SRCS})