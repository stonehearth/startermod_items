# CMAKE_BINARY_DIR is used for inclusion of automatically generated files
include_directories(. ../../Source/Shared ../../Source/Horde3dEngine ../../Source/Bindings/C++ ${CMAKE_BINARY_DIR} ${HORDE3D_EXTENSION_INCLUDE_DIRS})


add_definitions(-DCMAKE )

add_library(horde3d_stockpile STATIC
            decal_node.cpp
            stockpile_node.cpp
            toast_node.cpp
            )

target_link_libraries(horde3d_stockpile 
                      )
