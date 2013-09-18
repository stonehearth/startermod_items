FIND_PACKAGE(Cryptopp REQUIRED)
FIND_PACKAGE(Boost COMPONENTS filesystem REQUIRED)

include_directories(${Cryptopp_INCLUDE_DIRS})
include_directories(${Boost_INCLUDE_DIRS})

add_library(resources STATIC
            animation.cpp
            manifest.cpp
            res_manager.cpp
            )

target_link_libraries(resources
                      ${CppNetlib_LIBRARIES}
                     )