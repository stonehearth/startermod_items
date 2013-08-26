CMAKE_MINIMUM_REQUIRED(VERSION 2.8.5)
PROJECT(Stonehearth)

## Fix precompiled headers...
## See http://connect.microsoft.com/VisualStudio/feedback/details/746718/frequently-get-c1027-from-vc-100-compiler-after-installing-vs-2012-rc
string(REPLACE "[/\\-]Zm[0-9]+" " " CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})

## See http://connect.microsoft.com/VisualStudio/feedback/details/746718/frequently-get-c1027-from-vc-100-compiler-after-installing-vs-2012-rc
SET(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} /Ym0x20000000 ")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Ym0x20000000 ")

## Use MT instead of MD...
#SET(CMAKE_CXX_FLAGS_DEBUG "/D_DEBUG /MTd /Zi /Ob0 /Od /RTC1")
#SET(CMAKE_CXX_FLAGS_MINSIZEREL     "/MT /O1 /Ob1 /D NDEBUG")
#SET(CMAKE_CXX_FLAGS_RELEASE        "/MT /O2 /Ob2 /D NDEBUG")
#SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/MT /Zi /O2 /Ob1 /D NDEBUG")
#SET(CMAKE_C_FLAGS_DEBUG "/D_DEBUG /MTd /Zi /Ob0 /Od /RTC1")
#SET(CMAKE_C_FLAGS_MINSIZEREL     "/MT /O1 /Ob1 /D NDEBUG")
#SET(CMAKE_C_FLAGS_RELEASE        "/MT /O2 /Ob2 /D NDEBUG")
#SET(CMAKE_C_FLAGS_RELWITHDEBINFO "/MT /Zi /O2 /Ob1 /D NDEBUG")

#SET(CMAKE_CXX_FLAGS_DEBUG "/FR")
#SET(CMAKE_C_FLAGS_DEBUG "/FR")

# VC11 deprecates GX for EHsc
STRING(REPLACE "/GX" "/EHsc" CMAKE_C_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG})

# MSVC only...
SET(CMAKE_C_FLAGS "/DGTEST_HAS_TR1_TUPLE=0" ${CMAKE_C_FLAGS})
SET(CMAKE_CXX_FLAGS "/DGTEST_HAS_TR1_TUPLE=0" ${CMAKE_CXX_FLAGS})

# Add browse information 
# SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /FR")
#MESSAGE("sup" ${CMAKE_CXX_FLAGS})

SET(CMAKE_INSTALL_PREFIX "${Stonehearth_BINARY_DIR}/../install")


Function(PostBuildCopy SRCFILE DIRTO)
    get_filename_component(FILENAME SRCFILE NAME)
    get_filename_component(DIRFROM SRCFILE PATH)
    Add_Custom_Command(
        OUTPUT ${DIRTO}/${FILENAME}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DIRFROM}/${FILENAME} ${DIRTO}/${FILENAME}
        DEPENDS ${DIRFROM}/${FILENAME}
        COMMENT "Updating ${FILENAME} because it changed...")
        
    Set (POST_BUILD_OUTPUT_DEPS ${POST_BUILD_OUTPUT_DEPS} ${DIRTO}/${FILENAME} PARENT_SCOPE)
    Set ( ALL_SOURCES ${ALL_SOURCES} ${DIRFROM}/${FILENAME} PARENT_SCOPE)
EndFunction()

INCLUDE(RadiantPackageList.txt)

# almost everyone uses protocol buffers, so throw it in at the top
# level.  no, i'm not proud of this
FIND_PACKAGE(Protobuf REQUIRED)

FIND_PACKAGE(Gflags REQUIRED)
FIND_PACKAGE(Glog   REQUIRED)
FIND_PACKAGE(Snappy REQUIRED)
FIND_PACKAGE(Boost COMPONENTS filesystem REQUIRED)
FIND_PACKAGE(Lua REQUIRED)
FIND_PACKAGE(Luabind REQUIRED)
FIND_PACKAGE(GTest REQUIRED NO_MODULE)
FIND_PACKAGE(CppNetlib REQUIRED)
FIND_PACKAGE(Libjson REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${PROTOBUF_INCLUDE_DIRS})
include_directories(${Glog_INCLUDE_DIRS})
include_directories(${Gflags_INCLUDE_DIRS})
include_directories(${Snappy_INCLUDE_DIRS})
include_directories(${Lua_INCLUDE_DIRS})
include_directories(${Luabind_INCLUDE_DIRS})
include_directories(${GTest_INCLUDE_DIRS})
include_directories(${CppNetlib_INCLUDE_DIRS})
include_directories(${Libjson_INCLUDE_DIRS})

include_directories(.)
include_directories(include)

include_directories("c:/program files (x86)/microsoft directx sdk (june 2010)/include")

include_directories(${CMAKE_CURRENT_BINARY_DIR}) # why?
include_directories(../build/protocols) # xxx: BADDD
SET(Horde3D_INCLUDE_DIRS "${Stonehearth_SOURCE_DIR}/horde3d/Source/Shared" "${Stonehearth_SOURCE_DIR}/horde3d/Bindings/C++" "${Stonehearth_SOURCE_DIR}/horde3d/glfw")
include_directories(${Horde3D_INCLUDE_DIRS})

add_definitions(-DUNICODE -D_UNICODE -D_CRT_SECURE_NO_WARNINGS -wd4996)

add_subdirectory(core)
add_subdirectory(chromium)
add_subdirectory(client)
add_subdirectory(client_app)
add_subdirectory(csg)
add_subdirectory(game_engine)
add_subdirectory(horde3d)
add_subdirectory(lua)
add_subdirectory(metrics)
add_subdirectory(om)
add_subdirectory(physics)
add_subdirectory(platform)
add_subdirectory(protocols)
add_subdirectory(resources)
add_subdirectory(dm)
add_subdirectory(simulation)

