# Characters
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/Behaviors BEHAVIORS_SRCS)
ckbb_add_module(Characters SOURCES Characters.cpp Characters.rc ${BEHAVIORS_SRCS})