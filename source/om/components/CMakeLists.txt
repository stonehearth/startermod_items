FIND_PACKAGE(Lua REQUIRED)
FIND_PACKAGE(Luabind REQUIRED)
FIND_PACKAGE(Libjson REQUIRED)
FIND_PACKAGE(Cryptopp REQUIRED)
FIND_PACKAGE(Boost COMPONENTS filesystem REQUIRED)

include_directories(${Lua_INCLUDE_DIRS})
include_directories(${Luabind_INCLUDE_DIRS})
include_directories(${Libjson_INCLUDE_DIRS})
include_directories(${Cryptopp_INCLUDE_DIRS})
include_directories(${Boost_INCLUDE_DIRS})

add_library(om_components STATIC
            build_order.cpp
            build_order_dependencies.cpp
            grid_build_order.cpp
            region_build_order.cpp
            build_orders.cpp
            wall.cpp
            post.cpp
            scaffolding.cpp
            floor.cpp
            peaked_roof.cpp
            room.cpp
            portal.cpp
            fixture.cpp
            effect_list.cpp
            clock.cpp
            collision_shape.cpp
            component.cpp
            entity_container.cpp
            grid_collision_shape.cpp
            item.cpp
            mob.cpp
            render_grid.cpp
            render_rig.cpp
            render_info.cpp
            terrain.cpp
            unit_info.cpp
            region_collision_shape.cpp
            sphere_collision_shape.cpp
            stockpile_designation.cpp
            vertical_pathing_region.cpp
            user_behavior.cpp
            user_behavior_queue.cpp
            profession.cpp
            automation_task.cpp
            automation_queue.cpp
            paperdoll.cpp
            sensor_list.cpp
            attributes.cpp
            aura_list.cpp
            target_tables.cpp
            inventory.cpp
            weapon_info.cpp
			lua_components.cpp
			destination
            pch.cpp
            )

if (MSVC)
   set_target_properties(om_components PROPERTIES COMPILE_FLAGS "/Yupch.h")
   set_source_files_properties(pch.cpp PROPERTIES COMPILE_FLAGS "/Ycpch.h")
endif(MSVC)
