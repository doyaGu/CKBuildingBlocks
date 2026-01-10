# Materials
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/Behaviors BEHAVIORS_SRCS)
ckbb_add_module(Materials SOURCES Materials.cpp Materials.rc ${BEHAVIORS_SRCS})