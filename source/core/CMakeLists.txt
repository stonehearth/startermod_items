FIND_PACKAGE(Gflags     REQUIRED)
FIND_PACKAGE(Glog       REQUIRED)

add_library(core STATIC 
            config.cpp
            log.cpp
            log_win32.cpp
            )

target_link_libraries(core
                      ${Glog_LIBRARIES}
                      ${Gflags_LIBRARIES}
                      )
