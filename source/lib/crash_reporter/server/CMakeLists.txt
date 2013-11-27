FIND_PACKAGE(Breakpad REQUIRED)
FIND_PACKAGE(Poco REQUIRED)

include_directories(${Breakpad_INCLUDE_DIRS})
include_directories(${Poco_INCLUDE_DIRS})

add_executable(crash_reporter_server
               WIN32
               main_win32.cpp
               crash_reporter_server.cpp
               )

set_target_properties(crash_reporter_server
                      PROPERTIES OUTPUT_NAME
                      crash_reporter)

target_link_libraries(crash_reporter_server
                      core
                      ${Boost_LIBRARIES}
                      ${Breakpad_LIBRARIES}
                      ${Poco_LIBRARIES}
                      )
