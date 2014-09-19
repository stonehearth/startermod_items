
add_library(physics STATIC
            octtree.cpp
            physics_util.cpp
            free_motion.cpp
            nav_grid.cpp
            nav_grid_tile.cpp
            nav_grid_tile_data.cpp
            mob_tracker.cpp
            region_tracker.cpp
            collision_tracker.cpp
            terrain_tracker.cpp
            terrain_tile_tracker.cpp
            sensor_tracker.cpp
            sensor_tile_tracker.cpp
            destination_tracker.cpp
            movement_modifier_shape_tracker.cpp
            region_collision_shape_tracker.cpp
            vertical_pathing_region_tracker.cpp
            )

add_dependencies(physics
                 om_generated_headers)

