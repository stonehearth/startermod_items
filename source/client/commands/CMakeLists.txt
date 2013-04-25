FIND_PACKAGE(Boost 1.48.0 REQUIRED)
FIND_PACKAGE(Libjson REQUIRED)

include_directories(${Boost_INCLUDE_DIR})
include_directories(${Libjson_INCLUDE_DIRS})

add_library(client_commands STATIC
            create_room.cpp
            create_portal.cpp
            execute_action.cpp
            pending_command.cpp
            response.cpp
            trace_entity.cpp
            pch.cpp
            )

if (MSVC)
   set_target_properties(client_commands PROPERTIES COMPILE_FLAGS "/Yupch.h")
   set_source_files_properties(pch.cpp PROPERTIES COMPILE_FLAGS "/Ycpch.h")
endif(MSVC)

