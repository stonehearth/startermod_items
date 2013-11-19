FIND_PACKAGE(Lua REQUIRED)
FIND_PACKAGE(Luabind REQUIRED)
FIND_PACKAGE(Libjson REQUIRED)
FIND_PACKAGE(Cryptopp REQUIRED)

include_directories(${Lua_INCLUDE_DIRS})
include_directories(${Luabind_INCLUDE_DIRS})
include_directories(${Libjson_INCLUDE_DIRS})
include_directories(${Cryptopp_INCLUDE_DIRS})

add_library(csg_lua STATIC
            lua_csg.cpp
            lua_cube.cpp
            lua_point.cpp
            lua_region.cpp
            lua_heightmap.cpp
            lua_edgelist.cpp
            lua_quaternion.cpp
            lua_ray.cpp
            lua_random_number_generator.cpp
            pch.cpp
            )

if (MSVC)
   set_target_properties(csg_lua PROPERTIES COMPILE_FLAGS "/Yupch.h")
   set_source_files_properties(pch.cpp PROPERTIES COMPILE_FLAGS "/Ycpch.h")
endif(MSVC)
