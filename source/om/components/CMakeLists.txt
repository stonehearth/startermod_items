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
            pch.cpp
            attributes.cpp
            aura_list.cpp
            clock.cpp
            collision_shape.cpp
            component.cpp
            destination.cpp
            effect_list.cpp
            entity_container.cpp
            lua_components.cpp
            mob.cpp
            region_collision_shape.cpp
            render_info.cpp
            render_region.cpp
            model_variants.cpp
            sensor_list.cpp
            sphere_collision_shape.cpp
            target_tables.cpp
            terrain.cpp
            paperdoll.cpp
            carry_block.cpp
            vertical_pathing_region.cpp
            item.cpp
            unit_info.cpp
            )

if (MSVC)
   set_target_properties(om_components PROPERTIES COMPILE_FLAGS "/Yupch.h")
   set_source_files_properties(pch.cpp PROPERTIES COMPILE_FLAGS "/Ycpch.h")
endif(MSVC)
