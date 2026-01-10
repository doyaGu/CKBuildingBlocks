# MeshModifiers
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/Behaviors BEHAVIORS_SRCS)
ckbb_add_module(MeshModifiers SOURCES MeshModifiers.cpp MeshModifiers.rc ${BEHAVIORS_SRCS})
