FIND_PACKAGE(Lua REQUIRED)
FIND_PACKAGE(Luabind REQUIRED)

include_directories(${Lua_INCLUDE_DIRS})
include_directories(${Luabind_INCLUDE_DIRS})

add_library(csg STATIC
            color.cpp
            cube.cpp
            csg.cpp
            point.cpp
            region.cpp
            matrix3.cpp
            matrix4.cpp
            meshtools.cpp
            quaternion.cpp
            ray.cpp
            sphere.cpp
            transform.cpp
            voronoi.cpp
            voronoi_map.cpp
            heightmap.cpp
            util.cpp
            pch.cpp
            )

add_subdirectory(lua)

target_link_libraries(csg
                      csg_lua
                     )

add_executable(unittests_csg
               unittests.cpp)

target_link_libraries(unittests_csg
                      winmm.lib
                      ${Boost_LIBRARIES}
                      ${GTest_LIBRARIES}
                      csg
                      perfmon
                      core
                     )
if (MSVC)
   set_target_properties(csg PROPERTIES COMPILE_FLAGS "/Yupch.h")
   set_source_files_properties(pch.cpp PROPERTIES COMPILE_FLAGS "/Ycpch.h")
endif(MSVC)
