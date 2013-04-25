add_subdirectory(win32)

add_library(platform STATIC
            backtrace.cpp
            stackwalker.cpp
            thread_pool.cpp
            )

target_link_libraries(platform
                      winmm
                      platform_win32)
