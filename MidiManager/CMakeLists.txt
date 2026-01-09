# MidiManager

add_library(MidiManager SHARED
        MidiBehaviors.cpp
        MidiManager.cpp
        MidiManager.h
        MidiSound/midisound.cpp
        MidiSound/midisound.h
        MidiSound/muldiv32.h
        MidiSound/smf.cpp
        MidiSound/smf.h
        MidiSound/smfi.h
        MidiSound/smfread.cpp)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/Behaviors MIDIMANAGER_BEHAVIORS_SRCS)
target_sources(MidiManager PRIVATE ${MIDIMANAGER_BEHAVIORS_SRCS})
target_include_directories(MidiManager PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/MidiSound)
target_link_libraries(MidiManager PRIVATE CK2 VxMath winmm)

install(TARGETS MidiManager RUNTIME DESTINATION BuildingBlocks)
