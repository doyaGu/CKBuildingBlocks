# Characters

add_library(Characters SHARED Characters.cpp Characters.rc)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/Behaviors CHARACTERS_BEHAVIORS_SRCS)
target_sources(Characters PRIVATE ${CHARACTERS_BEHAVIORS_SRCS})
target_include_directories(Characters PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Characters PRIVATE CK2 VxMath)

install(TARGETS Characters RUNTIME DESTINATION BuildingBlocks)