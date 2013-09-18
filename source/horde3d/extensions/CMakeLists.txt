# CMAKE_BINARY_DIR is used for inclusion of automatically generated files
include_directories(. ../Source/Shared ../Source/Horde3dEngine ../Source/Bindings/C++ ${CMAKE_BINARY_DIR} ${HORDE3D_EXTENSION_INCLUDE_DIRS})

add_subdirectory(debug_shapes)
add_subdirectory(stockpile)
add_subdirectory(cubemitter)

add_definitions(-DCMAKE )

add_library(horde3d_extensions STATIC
            extension.cpp
            )
            
target_link_libraries(horde3d_extensions
                      horde3d_debug_shapes
                      horde3d_stockpile
                      horde3d_cubemitter
                      )