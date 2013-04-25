# CMAKE_BINARY_DIR is used for inclusion of automatically generated files
include_directories(. ../../Source/Shared ../../Source/Horde3dEngine ../../Source/Bindings/C++ ${CMAKE_BINARY_DIR} ${HORDE3D_EXTENSION_INCLUDE_DIRS})

add_definitions(-DCMAKE )

add_library(horde3d_debug_shapes STATIC
            debug_shapes.cpp
            )
