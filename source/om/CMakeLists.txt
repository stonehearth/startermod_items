FIND_PACKAGE(Lua REQUIRED)
FIND_PACKAGE(Luabind REQUIRED)
FIND_PACKAGE(Libjson REQUIRED)
FIND_PACKAGE(Cryptopp REQUIRED)

include_directories(${Lua_INCLUDE_DIRS})
include_directories(${Luabind_INCLUDE_DIRS})
include_directories(${Libjson_INCLUDE_DIRS})
include_directories(${Cryptopp_INCLUDE_DIRS})

add_subdirectory(components)
add_subdirectory(error_browser)
add_subdirectory(lua)

add_library(om STATIC
            entity.cpp
            om_alloc.cpp
            selection.cpp
            stonehearth.cpp
            tiled_region.cpp
            )

add_dependencies(om
                 protobuf_generated_headers)


target_link_libraries(om
                      om_lua
                      om_components
                      om_error_browser
                      )
