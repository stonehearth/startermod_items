FIND_PACKAGE(Libjson REQUIRED)
FIND_PACKAGE(Cryptopp REQUIRED)
FIND_PACKAGE(Boost COMPONENTS filesystem REQUIRED)

include_directories(${Libjson_INCLUDE_DIRS})
include_directories(${Cryptopp_INCLUDE_DIRS})
include_directories(${Boost_INCLUDE_DIRS})

add_library(resources STATIC
            action.cpp
            array_resource.cpp
            animation.cpp
            data_resource.cpp
            model.cpp
            object_resource.cpp
            res_manager.cpp
            rig.cpp
            region2d.cpp
            string_resource.cpp
            number_resource.cpp
            skeleton.cpp
            )

