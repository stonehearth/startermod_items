FIND_PACKAGE(Breakpad REQUIRED)
FIND_PACKAGE(Poco REQUIRED)

include_directories(${Breakpad_INCLUDE_DIRS})
include_directories(${Poco_INCLUDE_DIRS})

add_library(crash_reporter_client
            crash_reporter_client.cpp
            pch.cpp
            )

target_link_libraries(crash_reporter_client
                      ${Breakpad_LIBRARIES}
                      ${Poco_LIBRARIES}
                      )
