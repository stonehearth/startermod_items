# CMAKE_BINARY_DIR is used for inclusion of automatically generated files
include_directories(. ${Horde3D_INCLUDE_DIRS})

add_definitions(-DCMAKE )

add_library(horde3d_screenspace STATIC
            screenspace.cpp
            rect.cpp
            )
