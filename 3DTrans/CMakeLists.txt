# 3DTrans

# Build qhull as a static library
aux_source_directory(qhull QHULL_SRCS)
add_library(qhull STATIC ${QHULL_SRCS})
target_include_directories(qhull PUBLIC qhull)

add_library(3DTrans SHARED 3DTrans.cpp PortalsManager.cpp PortalsManager.h 3DTrans.rc)
set_target_properties(3DTrans PROPERTIES OUTPUT_NAME "3DTransfo")

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/Behaviors 3DTRANS_BEHAVIORS_SRCS)
target_sources(3DTrans PRIVATE ${3DTRANS_BEHAVIORS_SRCS})
target_include_directories(3DTrans PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(3DTrans PRIVATE CK2 VxMath qhull)

install(TARGETS 3DTrans RUNTIME DESTINATION BuildingBlocks)