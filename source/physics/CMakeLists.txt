
add_library(physics STATIC
            octtree.cpp
            nav_grid.cpp
            nav_grid_tile.cpp
            nav_grid_tile_data.cpp
            mob_tracker.cpp
            region_tracker.cpp
            collision_tracker.cpp
            terrain_tracker.cpp
            terrain_tile_tracker.cpp
            derived_region_tracker.cpp
            sensor_tracker.cpp
            sensor_tile_tracker.cpp
            )

add_dependencies(physics
                 om_generated_headers)

