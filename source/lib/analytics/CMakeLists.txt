FIND_PACKAGE(Libcurl REQUIRED)
FIND_PACKAGE(Cryptopp REQUIRED)
FIND_PACKAGE(Libjson REQUIRED)

include_directories(${Libcurl_INCLUDE_DIRS})
include_directories(${Libjson_INCLUDE_DIRS})
include_directories(${Cryptopp_INCLUDE_DIRS})

add_library(analytics
            analytics.cpp
            analytics_logger.cpp
            design_event.cpp
            event_data.cpp
            )

target_link_libraries(analytics 
                      ${Libcurl_LIBRARIES}
                      ${Cryptopp_LIBRARIES}
                      ${Libjson_LIBRARIES}
                      core
                      )