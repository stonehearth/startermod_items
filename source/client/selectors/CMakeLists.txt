
include_directories(${Horde3D_INCLUDE_DIRS})

add_library(client_selectors STATIC
            actor_selector.cpp
            voxel_range_selector.cpp
            pch.cpp
            )

if (MSVC)
   set_target_properties(client_selectors PROPERTIES COMPILE_FLAGS "/Yupch.h")
   set_source_files_properties(pch.cpp PROPERTIES COMPILE_FLAGS "/Ycpch.h")
endif(MSVC)
