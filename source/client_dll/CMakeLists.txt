FIND_PACKAGE(Boost 1.48.0 COMPONENTS system regex date_time REQUIRED)
FIND_PACKAGE(Gflags     REQUIRED)
FIND_PACKAGE(Glog       REQUIRED)
FIND_PACKAGE(Cryptopp   REQUIRED)
FIND_PACKAGE(Libjson REQUIRED)
FIND_PACKAGE(Lua REQUIRED)
FIND_PACKAGE(Luabind REQUIRED)

# FIND_PACKAGE(Bullet     REQUIRED NO_MODULE)

include_directories(${Boost_INCLUDE_DIR})
include_directories(${Libjson_INCLUDE_DIRS})
include_directories(${Lua_INCLUDE_DIRS})
include_directories(${Luabind_INCLUDE_DIRS})

add_library(client_dll SHARED
            dllmain.cpp
            )

target_link_libraries(client_dll
                             ${PROTOBUF_LIBRARIES}
                             ${Boost_LIBRARIES}
                             ${Glog_LIBRARIES}
                             ${Gflags_LIBRARIES}
                             ${Cryptopp_LIBRARIES}
                             ${Libjson_LIBRARIES}
                             comctl32
                             winmm
                             client
                             core
                             csg
                             dm
                             game_engine
                             metrics
                             om
                             physics
                             platform
                             protocols
                             resources
                             simulation
                             )
