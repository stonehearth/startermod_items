add_subdirectory(win32)

add_library(platform STATIC
            thread_pool.cpp
            FileWatcher.cpp
            sysinfo.cpp
            )

target_link_libraries(platform
                      winmm
                      version
                      platform_win32)
