FIND_PACKAGE(Boost COMPONENTS filesystem REQUIRED)
FIND_PACKAGE(Lua REQUIRED)
FIND_PACKAGE(Luabind REQUIRED)
FIND_PACKAGE(Libjson REQUIRED)
FIND_PACKAGE(Libnoise REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${Lua_INCLUDE_DIRS})
include_directories(${Luabind_INCLUDE_DIRS})
include_directories(${Libjson_INCLUDE_DIRS})
include_directories(${Libnoise_INCLUDE_DIRS})

add_library(simulation_script STATIC
            helpers.cpp
            script_host.cpp
            lua_basic_types.cpp
            lua_jobs.cpp
            lua_om.cpp
            lua_module.cpp
            lua_noise.cpp
            lua_resource.cpp
            pch.cpp
            )

if (MSVC)
   set_target_properties(simulation_script PROPERTIES COMPILE_FLAGS "/Yupch.h")
   set_source_files_properties(pch.cpp PROPERTIES COMPILE_FLAGS "/Ycpch.h")
endif(MSVC)


target_link_libraries(simulation_script
                      ${Boost_LIBRARIES}
                      ${Lua_LIBRARIES}
                      ${Luabind_LIBRARIES}
                      ${Libnoise_LIBRARIES}
                      )
