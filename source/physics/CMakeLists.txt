
add_library(physics STATIC
            octtree.cpp
            nav_grid.cpp
            )

add_dependencies(physics
                 om_generated_headers)

