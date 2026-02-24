# MidiManager
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/Behaviors BEHAVIORS_SRCS)
ckbb_add_module(MidiManager
        SOURCES
        MidiBehaviors.cpp
        MidiManager.cpp
        MidiManager.h
        MidiSound/MidiSound.cpp
        MidiSound/MidiSound.h
        MidiSound/muldiv32.h
        MidiSound/smf.cpp
        MidiSound/smf.h
        MidiSound/smfi.h
        MidiSound/smfread.cpp
        ${BEHAVIORS_SRCS}
        LINKS
        winmm
)
if (TARGET MidiManager)
    target_include_directories(MidiManager PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/MidiSound)
endif ()
if (TARGET MidiManagerStatic)
    target_include_directories(MidiManagerStatic PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/MidiSound)
endif ()
