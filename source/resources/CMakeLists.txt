FIND_PACKAGE(Libjson REQUIRED)
FIND_PACKAGE(Cryptopp REQUIRED)
FIND_PACKAGE(CppNetlib REQUIRED)
FIND_PACKAGE(Boost COMPONENTS filesystem REQUIRED)

include_directories(${Libjson_INCLUDE_DIRS})
include_directories(${Cryptopp_INCLUDE_DIRS})
include_directories(${CppNetlib_INCLUDE_DIRS})
include_directories(${Boost_INCLUDE_DIRS})

add_library(resources STATIC
            animation.cpp
            data_resource.cpp
            res_manager.cpp
            )

target_link_libraries(resources
                      ${CppNetlib_LIBRARIES}
                     )