add_library(simulation_jobs STATIC
			pch.cpp
            debug_colors.cpp
            job.cpp
            multi_path_finder.cpp
            path.cpp
            path_finder.cpp
            follow_path.cpp
            goto_location.cpp
            )

if (MSVC)
   set_target_properties(simulation_jobs PROPERTIES COMPILE_FLAGS "/Yupch.h")
   set_source_files_properties(pch.cpp PROPERTIES COMPILE_FLAGS "/Ycpch.h")
endif(MSVC)
