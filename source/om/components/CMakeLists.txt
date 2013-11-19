FIND_PACKAGE(Lua REQUIRED)
FIND_PACKAGE(Luabind REQUIRED)
FIND_PACKAGE(Libjson REQUIRED)
FIND_PACKAGE(Cryptopp REQUIRED)

include_directories(${Lua_INCLUDE_DIRS})
include_directories(${Luabind_INCLUDE_DIRS})
include_directories(${Libjson_INCLUDE_DIRS})
include_directories(${Cryptopp_INCLUDE_DIRS})

function(PROCESS_RADIANT_IDLS RIDL_TEMPLATE GENERATED_SRC)
  if(NOT ARGN)
    message(SEND_ERROR "Error: RIDL_GENERATE_CPP() called without any ridl files")
    return()
  endif()

  get_filename_component(ABS_RIDL_TEMPLATE ${RIDL_TEMPLATE} ABSOLUTE)
  get_filename_component(ABS_RIDLC ${STONEHEARTH_ROOT}/scripts/python/ridlc.py ABSOLUTE)
  set(${GENERATED_SRC})
  foreach(FIL ${ARGN})
    get_filename_component(ABS_FIL ${FIL} ABSOLUTE)
    get_filename_component(FIL_WE ${FIL} NAME_WE)
    list(APPEND ${GENERATED_SRC} "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.ridl.h")

    add_custom_command(
      OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.ridl.h"
      COMMAND  ${STONEHEARTH_ROOT}/scripts/python/ridlc.cmd
      ARGS "${CMAKE_CURRENT_BINARY_DIR}/${FIL_WE}.ridl.h" "${ABS_RIDL_TEMPLATE}" "${ABS_FIL}"
      DEPENDS ${ABS_FIL} ${ABS_RIDL_TEMPLATE} ${ABS_RIDLC}
      COMMENT "Running Radiant IDL compiler on ${FIL}"
      VERBATIM )
  endforeach()

  set_source_files_properties(${${GENERATED_SRC}} PROPERTIES GENERATED TRUE)
  set(${GENERATED_SRC} ${${GENERATED_SRC}} PARENT_SCOPE)
endfunction()

PROCESS_RADIANT_IDLS(component.ridl RIDL_SRC
                     carry_block.ridl.py
                     clock.ridl.py
                     )


add_library(om_components STATIC
            ${RIDL_SRC}
            pch.cpp
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
            vertical_pathing_region.cpp
            item.cpp
            unit_info.cpp
            )

if (MSVC)
   set_target_properties(om_components PROPERTIES COMPILE_FLAGS "/Yupch.h")
   set_source_files_properties(pch.cpp PROPERTIES COMPILE_FLAGS "/Ycpch.h")
endif(MSVC)
