FIND_PACKAGE(Libjson REQUIRED)
FIND_PACKAGE(Cryptopp REQUIRED)
FIND_PACKAGE(Boost COMPONENTS filesystem REQUIRED)

include_directories(${Libjson_INCLUDE_DIRS})
include_directories(${Cryptopp_INCLUDE_DIRS})
include_directories(${Boost_INCLUDE_DIRS})

add_library(httpd STATIC
            mongoose.c
            server.cpp
            )

