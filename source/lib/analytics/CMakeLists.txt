FIND_PACKAGE(Cryptopp REQUIRED)
FIND_PACKAGE(Libjson REQUIRED)
FIND_PACKAGE(Poco REQUIRED)

include_directories(${Cryptopp_INCLUDE_DIRS})
include_directories(${Libjson_INCLUDE_DIRS})
include_directories(${Poco_INCLUDE_DIRS})

add_library(analytics
            analytics.cpp
            analytics_logger.cpp
            design_event.cpp
            post_data.cpp
            event_data.cpp
            )

target_link_libraries(analytics 
                      ${Cryptopp_LIBRARIES}
                      ${Libjson_LIBRARIES}
                      ${Poco_LIBRARIES}
                      core
                      )