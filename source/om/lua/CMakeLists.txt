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

add_library(om_lua STATIC
            lua_attributes_component.cpp
            lua_aura_list_component.cpp
            lua_clock_component.cpp
            lua_component.cpp
            lua_effect_list_component.cpp
            lua_entity.cpp
            lua_entity_container_component.cpp
            lua_mob_component.cpp
            lua_om.cpp
            lua_region.cpp
            lua_region_collision_shape_component.cpp
            lua_render_info_component.cpp
            lua_model_variants_component.cpp
            lua_sensor_list_component.cpp
            lua_target_tables_component.cpp
            lua_sphere_collision_shape_component.cpp
            lua_terrain_component.cpp
            lua_vertical_pathing_region_component.cpp
            lua_lua_components_component.cpp
            lua_destination_component.cpp
            lua_render_region_component.cpp
            lua_paperdoll_component.cpp
            lua_carry_block_component.cpp
            lua_unit_info_component.cpp
            lua_item_component.cpp
            lua_data_binding.cpp
            pch.cpp
            )

if (MSVC)
   set_target_properties(om_components PROPERTIES COMPILE_FLAGS "/Yupch.h")
   set_source_files_properties(pch.cpp PROPERTIES COMPILE_FLAGS "/Ycpch.h")
endif(MSVC)
