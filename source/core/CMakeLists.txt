add_library(core STATIC 
            config.cpp
            log.cpp
            log_win32.cpp
            radiant_json.cpp
            )

target_link_libraries(core
                      )
