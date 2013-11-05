add_library(core STATIC 
            config.cpp
            log.cpp
            log_win32.cpp
            deferred.cpp
            guard.cpp
            thread.cpp
            process.cpp
            )

target_link_libraries(core
                      rpcrt4
                      )
