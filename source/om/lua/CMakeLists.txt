FIND_PACKAGE(Lua REQUIRED)
FIND_PACKAGE(Luabind REQUIRED)
FIND_PACKAGE(Libjson REQUIRED)
FIND_PACKAGE(Cryptopp REQUIRED)

include_directories(${Lua_INCLUDE_DIRS})
include_directories(${Luabind_INCLUDE_DIRS})
include_directories(${Libjson_INCLUDE_DIRS})
include_directories(${Cryptopp_INCLUDE_DIRS})

add_library(om_lua STATIC
            lua_component.cpp
            lua_entity.cpp
            lua_om.cpp
            lua_data_store.cpp
            lua_region.cpp
            pch.cpp
            )

add_dependencies(om_lua
                 om_generated_headers)

if (MSVC)
   set_target_properties(om_components PROPERTIES COMPILE_FLAGS "/Yupch.h")
   set_source_files_properties(pch.cpp PROPERTIES COMPILE_FLAGS "/Ycpch.h")
endif(MSVC)
