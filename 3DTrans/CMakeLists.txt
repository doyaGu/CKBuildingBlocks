# 3DTrans

# Build qhull as a static library
aux_source_directory(qhull QHULL_SRCS)
add_library(qhull STATIC ${QHULL_SRCS})
target_include_directories(qhull PUBLIC qhull)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/Behaviors BEHAVIORS_SRCS)
ckbb_add_module(3DTrans
        SOURCES 3DTrans.cpp PortalsManager.cpp PortalsManager.h 3DTrans.rc ${BEHAVIORS_SRCS}
        LINKS qhull
        OUTPUT_NAME 3DTransfo
)