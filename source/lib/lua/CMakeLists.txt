FIND_PACKAGE(Cryptopp   REQUIRED)
FIND_PACKAGE(Libjson REQUIRED)
FIND_PACKAGE(Lua REQUIRED)
FIND_PACKAGE(Luabind REQUIRED)

include_directories(${Boost_INCLUDE_DIR})
include_directories(${Libjson_INCLUDE_DIRS})
include_directories(${Lua_INCLUDE_DIRS})
include_directories(${Luabind_INCLUDE_DIRS})
include_directories(${Horde3D_INCLUDE_DIRS})

add_library(lua
            script_host.cpp
            lua_supplemental.cpp
            analytics/open_analytics.cpp
            audio/open_audio.cpp
            res/open_res.cpp
            sim/open_sim.cpp
            client/open_client.cpp
            rpc/open_rpc.cpp
            om/open_om.cpp
            dm/open_dm.cpp
            voxel/open_voxel.cpp
            pch.cpp
            )

add_dependencies(lua
                 om_generated_headers)

if (MSVC)
   set_target_properties(client PROPERTIES COMPILE_FLAGS "/Yupch.h")
   set_source_files_properties(pch.cpp PROPERTIES COMPILE_FLAGS "/Ycpch.h")
endif(MSVC)

target_link_libraries(lua
                      ${PROTOBUF_LIBRARIES}
                      ${Boost_LIBRARIES}
                      ${Python_LIBRARIES}
                      ${Cryptopp_LIBRARIES}
                      ${Libjson_LIBRARIES}
                      ${Lua_LIBRARIES}
                      ${Luabind_LIBRARIES}
                      )

