add_library(simulation_jobs STATIC
			pch.cpp
            debug_colors.cpp
            job.cpp
            path.cpp
            path_finder.cpp
            path_finder_src.cpp
            path_finder_dst.cpp
            follow_path.cpp
            goto_location.cpp
            lua_job.cpp
            )

add_dependencies(simulation_jobs
                 protobuf_generated_headers)

if (MSVC)
   set_target_properties(simulation_jobs PROPERTIES COMPILE_FLAGS "/Yupch.h")
   set_source_files_properties(pch.cpp PROPERTIES COMPILE_FLAGS "/Ycpch.h")
endif(MSVC)
