FIND_PACKAGE(Libjson REQUIRED)
FIND_PACKAGE(Libclaw REQUIRED)
FIND_PACKAGE(Sfml REQUIRED)

include_directories(${Horde3D_INCLUDE_DIRS})
include_directories(${Libjson_INCLUDE_DIRS})
include_directories(${Libclaw_INCLUDE_DIRS})
include_directories(${Sfml_INCLUDE_DIRS})

add_library(client_renderer STATIC
            pch.cpp
            pipeline.cpp
            skeleton.cpp
            renderer.cpp
            render_entity.cpp
            render_entity_container.cpp
            render_mob.cpp
            render_terrain.cpp
            render_effect_list.cpp
            render_destination.cpp
            render_carry_block.cpp
            render_render_info.cpp
            render_util.cpp
            render_vertical_pathing_region.cpp
            render_lua_component.cpp
            lua_renderer.cpp
            lua_render_entity.cpp
            camera.cpp
            perfhud/perfhud.cpp
            perfhud/timeline.cpp
            perfhud/timeline_column.cpp
            perfhud/render_context.cpp
            )

add_dependencies(client_renderer
                 om_generated_headers
                 protobuf_generated_headers)

if (MSVC)
   set_target_properties(client_renderer PROPERTIES COMPILE_FLAGS "/Yupch.h")
   set_source_files_properties(pch.cpp PROPERTIES COMPILE_FLAGS "/Ycpch.h")
endif(MSVC)

target_link_libraries(client_renderer
                      Horde3D
                      voxel
                      ${Libclaw_LIBRARIES}
                      ${Sfml_LIBRARIES}
                     )
