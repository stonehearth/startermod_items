FIND_PACKAGE(Boost COMPONENTS filesystem REQUIRED)
FIND_PACKAGE(Libjson REQUIRED)
FIND_PACKAGE(Lua REQUIRED)
FIND_PACKAGE(Luabind REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${Protothread_INCLUDE_DIRS})
include_directories(${Libjson_INCLUDE_DIRS})
include_directories(${Lua_INCLUDE_DIRS})
include_directories(${Luabind_INCLUDE_DIRS})


add_subdirectory(actions)
add_subdirectory(jobs)
# add_subdirectory(scheduler)
# add_subdirectory(script)

add_library(simulation STATIC
            interfaces.cpp
            simulation.cpp
               pch.cpp
            )

if (MSVC)
   set_target_properties(simulation PROPERTIES COMPILE_FLAGS "/Yupch.h")
   set_source_files_properties(pch.cpp PROPERTIES COMPILE_FLAGS "/Ycpch.h")
endif(MSVC)

# also have to turn off /RTC1.  ug!
#SET_SOURCE_FILES_PROPERTIES(simplexnoise1234.cpp PROPERTIES COMPILE_FLAGS -O2)

target_link_libraries(simulation
                      resources
                      simulation_actions
                      simulation_jobs
                      )
