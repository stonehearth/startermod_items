FIND_PACKAGE(Breakpad REQUIRED)

include_directories(${Breakpad_INCLUDE_DIRS})

add_executable(crash_reporter
               WIN32
               crash_reporter.cpp
               pch.cpp
               )

target_link_libraries(crash_reporter 
                      ${Breakpad_LIBRARIES}
                      )