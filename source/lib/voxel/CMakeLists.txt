add_library(voxel
            pch.cpp
            qubicle_file.cpp
            qubicle_brush.cpp
            fabricator.cpp
            voxel_util.cpp
            nine_grid_brush.cpp
            )

if (MSVC)
   set_target_properties(client PROPERTIES COMPILE_FLAGS "/Yupch.h")
   set_source_files_properties(pch.cpp PROPERTIES COMPILE_FLAGS "/Ycpch.h")
endif(MSVC)
