# Controllers
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/Behaviors BEHAVIORS_SRCS)
ckbb_add_module(Controllers SOURCES Controllers.cpp Controllers.rc ${BEHAVIORS_SRCS})