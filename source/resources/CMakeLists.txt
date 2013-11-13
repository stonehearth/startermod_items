FIND_PACKAGE(Cryptopp REQUIRED)
FIND_PACKAGE(Poco REQUIRED)

include_directories(${Cryptopp_INCLUDE_DIRS})
include_directories(${Poco_INCLUDE_DIRS})

add_library(resources STATIC
            animation.cpp
            manifest.cpp
            directory_module.cpp
            zip_module.cpp
            res_manager.cpp
            )

target_link_libraries(resources
                      ${Poco_LIBRARIES}
                     )