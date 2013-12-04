FIND_PACKAGE(Lua REQUIRED)
FIND_PACKAGE(Luabind REQUIRED)

include_directories(${Lua_INCLUDE_DIRS})
include_directories(${Luabind_INCLUDE_DIRS})

add_library(simulation_actions STATIC
            action.cpp
            run_toward_point_action.cpp
               pch.cpp
            )

add_dependencies(simulation_actions
                 om_generated_headers)


if (MSVC)
   set_target_properties(simulation_actions PROPERTIES COMPILE_FLAGS "/Yupch.h")
   set_source_files_properties(pch.cpp PROPERTIES COMPILE_FLAGS "/Ycpch.h")
endif(MSVC)

