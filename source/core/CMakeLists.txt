FIND_PACKAGE(Poco REQUIRED)

include_directories(${Poco_INCLUDE_DIRS})

add_library(core STATIC 
            config.cpp
            log.cpp
            log_win32.cpp
            deferred.cpp
            guard.cpp
            process.cpp
            )

target_link_libraries(core
                      rpcrt4
                      ${Poco_LIBRARIES}
                      )
