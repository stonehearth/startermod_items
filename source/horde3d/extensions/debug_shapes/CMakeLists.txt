# CMAKE_BINARY_DIR is used for inclusion of automatically generated files
include_directories(${Horde3D_INCLUDE_DIRS})

add_definitions(-DCMAKE )

add_library(horde3d_debug_shapes STATIC
            debug_shapes.cpp
            )

add_dependencies(horde3d_debug_shapes
                 protobuf_generated_headers)
