# Sounds
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/Behaviors BEHAVIORS_SRCS)
ckbb_add_module(Sounds SOURCES SoundBehavior.cpp ${BEHAVIORS_SRCS} LINKS winmm)