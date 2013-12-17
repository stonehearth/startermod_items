FIND_PACKAGE(ChromiumEmbedded REQUIRED)
FIND_PACKAGE(Cryptopp   REQUIRED)
FIND_PACKAGE(Libjson REQUIRED)
FIND_PACKAGE(Lua REQUIRED)
FIND_PACKAGE(Luabind REQUIRED)
FIND_PACKAGE(Breakpad REQUIRED)
FIND_PACKAGE(Poco REQUIRED)
FIND_PACKAGE(Sfml REQUIRED)

include_directories(${Boost_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${ChromiumEmbedded_INCLUDE_DIRS})
include_directories(${Horde3D_INCLUDE_DIRS})
include_directories(${Libjson_INCLUDE_DIRS})
include_directories(${Lua_INCLUDE_DIRS})
include_directories(${Luabind_INCLUDE_DIRS})
include_directories(${Breakpad_INCLUDE_DIRS})
include_directories(${Poco_INCLUDE_DIRS})
include_directories(${Sfml_INCLUDE_DIRS})

add_subdirectory(renderer)

add_library(client
            application.cpp
            clock.cpp
            client.cpp
            main.cpp
            xz_region_selector.cpp
            pch.cpp
            )

add_dependencies(client
                 om_generated_headers
                 protobuf_generated_headers)

if (MSVC)
   set_target_properties(client PROPERTIES COMPILE_FLAGS "/Yupch.h")
   set_source_files_properties(pch.cpp PROPERTIES COMPILE_FLAGS "/Ycpch.h")
endif(MSVC)

target_link_libraries(client 
                      ${PROTOBUF_LIBRARIES}
                      ${Boost_LIBRARIES}
                      ${Python_LIBRARIES}
                      ${Cryptopp_LIBRARIES}
                      ${Libjson_LIBRARIES}
                      ${Breakpad_LIBRARIES}
                      ${Poco_LIBRARIES}
                      ${Sfml_LIBRARIES}
                      comctl32
                      winmm
                      chromium_browser
                      client_renderer
                      core
                      crash_reporter_client
                      csg
                      dm
                      glfw
                      Horde3D
                      Horde3DUtils
                      horde3d_extensions
                      lua
                      metrics
                      om
                      physics
                      platform
                      protobuf
                      resources
                      simulation
                      rpc
                     )
