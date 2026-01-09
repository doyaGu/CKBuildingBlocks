
# MeshModifiers

add_library(MeshModifiers SHARED MeshModifiers.cpp MeshModifiers.rc)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/Behaviors MESHMODIFIERS_BEHAVIORS_SRCS)
target_sources(MeshModifiers PRIVATE ${MESHMODIFIERS_BEHAVIORS_SRCS})
target_include_directories(MeshModifiers PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(MeshModifiers PRIVATE CK2 VxMath)
