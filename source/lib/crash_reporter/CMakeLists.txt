FIND_PACKAGE(Breakpad REQUIRED)
FIND_PACKAGE(Poco REQUIRED)

include_directories(${Breakpad_INCLUDE_DIRS})
include_directories(${Poco_INCLUDE_DIRS})

add_executable(crash_reporter
               WIN32
               crash_reporter.cpp
               pch.cpp
               )

target_link_libraries(crash_reporter 
                      ${Boost_LIBRARIES}
                      ${Breakpad_LIBRARIES}
                      ${Poco_LIBRARIES}
                      )
