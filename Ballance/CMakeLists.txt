# Ballance

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/Behaviors BEHAVIORS_SRCS)
ckbb_add_module(Ballance
        SOURCES Ballance.cpp
                BallanceMessages.h
                BallanceMessages.cpp
                BallanceHelpers.h
                BallanceHelpers.cpp
                ${BEHAVIORS_SRCS}
)

if (BUILD_TESTING)
        add_executable(BallanceMessageMapTest
                tests/BallanceMessageMapTest.cpp
                BallanceMessages.cpp
        )
        target_include_directories(BallanceMessageMapTest PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
        add_test(NAME BallanceMessageMapTest COMMAND BallanceMessageMapTest)
endif ()
