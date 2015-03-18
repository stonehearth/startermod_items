CMAKE_MINIMUM_REQUIRED(VERSION 2.8.5)
#set(CMAKE_USER_MAKE_RULES_OVERRIDE     ${CMAKE_CURRENT_SOURCE_DIR}/make/c_flag_overrides.cmake)
#set(CMAKE_USER_MAKE_RULES_OVERRIDE_CXX ${CMAKE_CURRENT_SOURCE_DIR}/make/cxx_flag_overrides.cmake)

PROJECT(Stonehearth)

## Fix precompiled headers...
## See http://connect.microsoft.com/VisualStudio/feedback/details/746718/frequently-get-c1027-from-vc-100-compiler-after-installing-vs-2012-rc
#string(REPLACE "/Zm[0-9]+" "/Zm200" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
SET(CMAKE_C_FLAGS   " /Zm200${CMAKE_C_FLAGS}")
SET(CMAKE_CXX_FLAGS " /Zm200${CMAKE_CXX_FLAGS}")

if (DISABLE_WHOLE_PROGRAM_OPTIMIZATION)
   MESSAGE("whole program optimization is disabled")
else()
   MESSAGE("whole program optimization is enabled")
endif()   

## See http://connect.microsoft.com/VisualStudio/feedback/details/746718/frequently-get-c1027-from-vc-100-compiler-after-installing-vs-2012-rc
SET(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} /Ym0x20000000 ")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Ym0x20000000 ")

# Turn on multi processor builds
SET(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} /MP ")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP ")

# Work around C1128 (see https://msdn.microsoft.com/en-us/library/8578y171.aspx)
if("${RADIANT_BUILD_PLATFORM}" STREQUAL "x64")
MESSAGE("x64 /bigobj enabled")
SET(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} /bigobj ")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj ")
endif()

#SET(CMAKE_CXX_FLAGS_DEBUG "/FR")
#SET(CMAKE_C_FLAGS_DEBUG "/FR")

# VC11 deprecates GX for EHsc
STRING(REPLACE "/GX" "/EHsc" CMAKE_C_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG})

# Add enhanced debug information for optimized builds
SET(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /d2Zi+ ")
SET(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} /d2Zi+ ")

# Enable MemPro
IF(ENABLE_MEMPRO)
   MESSAGE(" ** MemPro support is on **")
   SET(CMAKE_C_FLAGS "/DENABLE_MEMPRO" ${CMAKE_C_FLAGS})
   SET(CMAKE_CXX_FLAGS "/DENABLE_MEMPRO" ${CMAKE_CXX_FLAGS})
ENDIF()

# Enable Object Counting
IF(ENABLE_OBJECT_COUNTER)
   MESSAGE(" ** Object counter support is on **")
   SET(CMAKE_C_FLAGS "/DENABLE_OBJECT_COUNTER" ${CMAKE_C_FLAGS})
   SET(CMAKE_CXX_FLAGS "/DENABLE_OBJECT_COUNTER" ${CMAKE_CXX_FLAGS})
ENDIF()

# Turn off MSVC security checks.
SET(CMAKE_C_FLAGS " /GS- ${CMAKE_C_FLAGS} ")
SET(CMAKE_CXX_FLAGS " /GS- ${CMAKE_CXX_FLAGS} ")


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

SET(RADIANT_ROOT              "$ENV{RADIANT_ROOT}")
#MESSAGE("Radiant root is ${RADIANT_ROOT}")
#string(REGEX REPLACE "\\\\" "/" RADIANT_ROOT ${RADIANT_ROOT} )
#
#MESSAGE("Radiant Root is: ${RADIANT_ROOT}")

SET(STONEHEARTH_ROOT          ${CMAKE_SOURCE_DIR})
MESSAGE("Stonehearth Root is: ${STONEHEARTH_ROOT}")

SET(RADIANT_MODULE_ROOT "${STONEHEARTH_ROOT}/modules")
INCLUDE(${RADIANT_MODULE_ROOT}/boost/cmake/config.txt)
INCLUDE(${RADIANT_MODULE_ROOT}/breakpad/cmake/config.txt)
INCLUDE(${RADIANT_MODULE_ROOT}/chromium-embedded/cmake/config.txt)
INCLUDE(${RADIANT_MODULE_ROOT}/cryptopp/cmake/config.txt)
INCLUDE(${RADIANT_MODULE_ROOT}/googletest/cmake/config.txt)
INCLUDE(${RADIANT_MODULE_ROOT}/libclaw/cmake/config.txt)
INCLUDE(${RADIANT_MODULE_ROOT}/libjson/cmake/config.txt)
INCLUDE(${RADIANT_MODULE_ROOT}/libpng/cmake/config.txt)
INCLUDE(${RADIANT_MODULE_ROOT}/lua/cmake/config.txt)
INCLUDE(${RADIANT_MODULE_ROOT}/luajit/cmake/config.txt)
INCLUDE(${RADIANT_MODULE_ROOT}/luabind/cmake/config.txt)
INCLUDE(${RADIANT_MODULE_ROOT}/poco/cmake/config.txt)
INCLUDE(${RADIANT_MODULE_ROOT}/protobuf/cmake/config.txt)
INCLUDE(${RADIANT_MODULE_ROOT}/sfml/cmake/config.txt)
INCLUDE(${RADIANT_MODULE_ROOT}/snappy/cmake/config.txt)
INCLUDE(${RADIANT_MODULE_ROOT}/eastl/cmake/config.txt)
INCLUDE(${RADIANT_MODULE_ROOT}/tbb/cmake/config.txt)

set(Boost_USE_STATIC_LIBS     ON)
set(Boost_USE_STATIC_RUNTIME  ON)
set(Boost_USE_MULTITHREADED   ON)

# this must be first, to pickup build_overrides.h before the one in source/include
MESSAGE("looking for build_overrids.h in ${CMAKE_CURRENT_BINARY_DIR}")
FIND_PATH(BUILD_OVERRIDES build_overrides.h PATHS "${CMAKE_CURRENT_BINARY_DIR}")
if (BUILD_OVERRIDES)
   MESSAGE("using build_overrides")
   INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
   ADD_DEFINITIONS(-DUSE_BUILD_OVERRIDES)
endif()

FIND_PACKAGE(Boost 1.48.0 COMPONENTS filesystem thread system regex date_time program_options log log_setup chrono REQUIRED)
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})

add_definitions( -DCURL_STATICLIB )

# to get EASTL to get the correct definition for wide characters
add_definitions( -DEA_CHAR16_NATIVE=1 )

# Treat all warnings as errors
add_definitions( -WX )

IF(NO_ASSERT)
   MESSAGE(" ** ASSERT's have been turned off!! **")
   add_definitions( -DNO_ASSERT )
ENDIF()

IF(VTUNE_PATH)
   add_definitions( -DVTUNE_PATH=${VTUNE_PATH} )
   SET(VTune_INCLUDE_DIRS "${VTUNE_PATH}/include")
   IF( "x64" STREQUAL "${RADIANT_BUILD_PLATFORM}")
      SET(VTune_LIBRARIES "${VTUNE_PATH}/lib64/libittnotify.lib")
   ELSE()
      SET(VTune_LIBRARIES "${VTUNE_PATH}/lib32/libittnotify.lib")
   ENDIF()
   MESSAGE(" ** VTune APIs have been turned on **")
   MESSAGE(" **    include: ${VTune_INCLUDE_DIRS}  **")
   MESSAGE(" **    libs:    ${VTune_LIBRARIES}  **")
ENDIF()

# Add /LTCG to speed up builds with WPO.
SET(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} /LTCG")

add_subdirectory(source)


