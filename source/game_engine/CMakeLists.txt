FIND_PACKAGE(Boost 1.48.0 COMPONENTS system)
FIND_PACKAGE(Lua REQUIRED)
FIND_PACKAGE(Luabind REQUIRED)
FIND_PACKAGE(Lpeg REQUIRED)

include_directories(${Boost_INCLUDE_DIR})
include_directories(${Lua_INCLUDE_DIRS})
include_directories(${Luabind_INCLUDE_DIRS})

add_library(game_engine STATIC
            arbiter.cpp
            client.cpp
            )

target_link_libraries(game_engine
					  ${Lpeg_LIBRARIES})