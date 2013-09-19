FIND_PACKAGE(Cryptopp REQUIRED)

include_directories(${Cryptopp_INCLUDE_DIRS})

add_library(resources STATIC
            animation.cpp
            res_manager.cpp
            )

target_link_libraries(resources
                     )