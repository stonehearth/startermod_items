FIND_PACKAGE(Libjson REQUIRED)
FIND_PACKAGE(Libclaw REQUIRED)

include_directories(${Horde3D_INCLUDE_DIRS})
include_directories(${Libjson_INCLUDE_DIRS})
include_directories(${Libclaw_INCLUDE_DIRS})

add_library(client_renderer STATIC
            geometry_generator.cpp
            pipeline.cpp
            skeleton.cpp
            renderer.cpp
#            render_floor.cpp
            render_carry_block.cpp
            render_build_order.cpp
            render_build_orders.cpp
            render_grid.cpp
#           render_ladder.cpp
            render_entity.cpp
            render_entity_container.cpp
            render_mob.cpp
            render_rig.cpp
            render_terrain.cpp
            render_effect_list.cpp
#            render_rig_map.cpp
            render_scaffolding.cpp
#            render_sensor.cpp
#            render_structure.cpp
            render_destination.cpp
            render_stockpile.cpp
#            render_storey.cpp
            texture_color_mapper.cpp
            qubicle_file.cpp
            render_paperdoll.cpp
            pch.cpp
            )

if (MSVC)
   set_target_properties(client_renderer PROPERTIES COMPILE_FLAGS "/Yupch.h")
   set_source_files_properties(pch.cpp PROPERTIES COMPILE_FLAGS "/Ycpch.h")
endif(MSVC)

target_link_libraries(client_renderer
                      Horde3D
                      ${Libclaw_LIBRARIES}
                     )