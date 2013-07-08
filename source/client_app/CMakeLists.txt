FIND_PACKAGE(Boost 1.48.0 COMPONENTS system regex date_time REQUIRED)
FIND_PACKAGE(Gflags     REQUIRED)
FIND_PACKAGE(Glog       REQUIRED)
FIND_PACKAGE(Cryptopp   REQUIRED)
FIND_PACKAGE(Libjson REQUIRED)
FIND_PACKAGE(Lua REQUIRED)
FIND_PACKAGE(Luabind REQUIRED)
FIND_PACKAGE(ChromiumEmbedded REQUIRED)

# FIND_PACKAGE(Bullet     REQUIRED NO_MODULE)

include_directories(${Boost_INCLUDE_DIR})
include_directories(${Libjson_INCLUDE_DIRS})
include_directories(${Lua_INCLUDE_DIRS})
include_directories(${Luabind_INCLUDE_DIRS})

add_executable(client_app
               WIN32
               main_win32.cpp
               )

# FILE(COPY ${ChromiumEmbedded_BINARY_FILES} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
                   #                   COMMAND echo copy ${ChromiumEmbedded_BINARIES}/<$CONFIGURATION>/* <$TARGET_DIR:client_app> -------------------------- $<CONFIGURATION>)

target_link_libraries(client_app
                             ${PROTOBUF_LIBRARIES}
                             ${Boost_LIBRARIES}
                             ${Glog_LIBRARIES}
                             ${Gflags_LIBRARIES}
                             ${Cryptopp_LIBRARIES}
                             ${Libjson_LIBRARIES}
                             comctl32
                             winmm
                             client
                             chromium_browser
                             core
                             csg
                             dm
                             game_engine
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

FILE(TO_NATIVE_PATH ${Stonehearth_SOURCE_DIR} STAGE_SCRIPT)
add_custom_command(TARGET client_app
                   POST_BUILD
                   COMMAND ${STAGE_SCRIPT}\\stage.cmd $<CONFIGURATION> $<TARGET_FILE_DIR:client_app>)
