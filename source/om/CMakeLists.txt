FIND_PACKAGE(Lua REQUIRED)
FIND_PACKAGE(Luabind REQUIRED)
FIND_PACKAGE(Libjson REQUIRED)
FIND_PACKAGE(Cryptopp REQUIRED)

include_directories(${Lua_INCLUDE_DIRS})
include_directories(${Luabind_INCLUDE_DIRS})
include_directories(${Libjson_INCLUDE_DIRS})
include_directories(${Cryptopp_INCLUDE_DIRS})

add_subdirectory(components)
add_subdirectory(object_formatter)
add_subdirectory(error_browser)
add_subdirectory(lua)

add_library(om STATIC
            pch.cpp
            entity.cpp
            om_alloc.cpp
            region.cpp
            selection.cpp
            stonehearth.cpp
            )

add_dependencies(om
                 protobuf_generated_headers)

if (MSVC)
   set_target_properties(om PROPERTIES COMPILE_FLAGS "/Yupch.h")
   set_source_files_properties(pch.cpp PROPERTIES COMPILE_FLAGS "/Ycpch.h")
endif(MSVC)


target_link_libraries(om
                      om_lua
                      om_components
                      om_object_formatter
                      om_error_browser
                      )
