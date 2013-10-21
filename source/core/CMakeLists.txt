add_library(core STATIC 
            config.cpp
            log.cpp
            log_win32.cpp
            deferred.cpp
            guard.cpp
            )

target_link_libraries(core
                      )
