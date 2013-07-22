FIND_PACKAGE(Boost 1.48.0 COMPONENTS system regex date_time program_options REQUIRED)
FIND_PACKAGE(Gflags     REQUIRED)
FIND_PACKAGE(Glog       REQUIRED)
FIND_PACKAGE(Cryptopp   REQUIRED)
FIND_PACKAGE(Libjson REQUIRED)
FIND_PACKAGE(Lua REQUIRED)
FIND_PACKAGE(Luabind REQUIRED)
# FIND_PACKAGE(Bullet     REQUIRED NO_MODULE)

include_directories(${Boost_INCLUDE_DIR})
include_directories(${Horde3D_INCLUDE_DIRS})
include_directories(${Libjson_INCLUDE_DIRS})
include_directories(${Lua_INCLUDE_DIRS})
include_directories(${Luabind_INCLUDE_DIRS})

add_subdirectory(renderer)
add_subdirectory(selectors)

add_library(client
            application.cpp
            client.cpp
            entity_traces.cpp
            main.cpp
            pch.cpp
            )

if (MSVC)
   set_target_properties(client PROPERTIES COMPILE_FLAGS "/Yupch.h")
   set_source_files_properties(pch.cpp PROPERTIES COMPILE_FLAGS "/Ycpch.h")
endif(MSVC)

target_link_libraries(client 
                             ${PROTOBUF_LIBRARIES}
                             ${Boost_LIBRARIES}
                             ${Glog_LIBRARIES}
                             ${Gflags_LIBRARIES}
                             ${Python_LIBRARIES}
                             ${Cryptopp_LIBRARIES}
                             ${Libjson_LIBRARIES}

#                             ${Bullet_LIBRARIES}
                             comctl32
                             winmm
#                             "C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)/Lib/x86/d3dx9.lib"
#                             "C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)/Lib/x86/d3d9.lib"
                             chromium_browser
                             client_renderer
                             client_selectors
                             core
                             csg
                             dm
                             game_engine
                             glfw
                             Horde3D
                             Horde3DUtils
                             horde3d_extensions
                             lua
                             math3d_common
                             math3d_collision
                             metrics
                             om
                             physics
                             platform
                             protocols
                             resources
                             simulation
                             )
