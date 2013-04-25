FIND_PACKAGE(Libjson REQUIRED)
FIND_PACKAGE(Cryptopp REQUIRED)
FIND_PACKAGE(Boost COMPONENTS filesystem REQUIRED)

include_directories(${Libjson_INCLUDE_DIRS})
include_directories(${Cryptopp_INCLUDE_DIRS})
include_directories(${Boost_INCLUDE_DIRS})

add_library(om_grid STATIC
            bricklib.cpp
            grid.cpp
            grid_tile.cpp
            iterator.cpp
            tile_address.cpp
            pch.cpp
            )

if (MSVC)
   set_target_properties(om_grid PROPERTIES COMPILE_FLAGS "/Yupch.h")
   set_source_files_properties(pch.cpp PROPERTIES COMPILE_FLAGS "/Ycpch.h")
endif(MSVC)
