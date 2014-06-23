add_library(simulation_jobs STATIC
            pch.cpp
            apply_free_movement.cpp
            debug_colors.cpp
            job.cpp
            task.cpp
            path.cpp
            bfs_path_finder.cpp
            a_star_path_finder.cpp
            direct_path_finder.cpp
            path_finder_src.cpp
            path_finder_dst.cpp
            entity_job_scheduler.cpp
            follow_path.cpp
            bump_location.cpp
            lua_job.cpp
            movement_helpers.cpp
            )

add_dependencies(simulation_jobs
                 protobuf_generated_headers)

if (MSVC)
   set_target_properties(simulation_jobs PROPERTIES COMPILE_FLAGS "/Yupch.h")
   set_source_files_properties(pch.cpp PROPERTIES COMPILE_FLAGS "/Ycpch.h")
endif(MSVC)
