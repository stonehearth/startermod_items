# almost everyone uses protocol buffers, so throw it in at the top
# level.  no, i'm not proud of this
FIND_PACKAGE(Protobuf REQUIRED)

FIND_PACKAGE(Lua REQUIRED)
FIND_PACKAGE(Luabind REQUIRED)
FIND_PACKAGE(GTest REQUIRED NO_MODULE)
FIND_PACKAGE(Libjson REQUIRED)
FIND_PACKAGE(Snappy REQUIRED)

include_directories(${PROTOBUF_INCLUDE_DIRS})
include_directories(${Snappy_INCLUDE_DIRS})
include_directories(${Lua_INCLUDE_DIRS})
include_directories(${Luabind_INCLUDE_DIRS})
include_directories(${GTest_INCLUDE_DIRS})
include_directories(${Libjson_INCLUDE_DIRS})

include_directories(.)
include_directories(include)

include_directories("c:/program files (x86)/microsoft directx sdk (june 2010)/include")

include_directories(${CMAKE_BINARY_DIR}/source)

SET(Horde3D_ROOT_DIR "${Stonehearth_SOURCE_DIR}/source/horde3d/")
SET(Horde3D_INCLUDE_DIRS "${Horde3D_ROOT_DIR}/Source/Shared" "${Horde3D_ROOT_DIR}/Source/Horde3dEngine" "${Horde3D_ROOT_DIR}/Bindings/C++" "${Horde3D_ROOT_DIR}/glfw/include/GLFW" "${Horde3D_ROOT_DIR}/extensions/")

add_definitions(-D_CRT_SECURE_NO_WARNINGS -wd4996)

add_subdirectory(core)
add_subdirectory(chromium)
add_subdirectory(client)
add_subdirectory(stonehearth)
add_subdirectory(csg)
add_subdirectory(lib)
add_subdirectory(horde3d)
add_subdirectory(metrics)
add_subdirectory(om)
add_subdirectory(physics)
add_subdirectory(platform)
add_subdirectory(protocols)
add_subdirectory(resources)
add_subdirectory(dm)
add_subdirectory(simulation)
add_subdirectory(type_registry)


