FIND_PACKAGE(ChromiumEmbedded REQUIRED)
FIND_PACKAGE(Boost 1.48.0 REQUIRED)

include_directories(${Boost_INCLUDE_DIR})
include_directories(${ChromiumEmbedded_INCLUDE_DIRS})

add_library(client_chromium STATIC
            buffered_response.cpp
            chromium.cpp
            pch.cpp
            )

if (MSVC)
   set_target_properties(client_chromium PROPERTIES COMPILE_FLAGS "/Yupch.h")
   set_source_files_properties(pch.cpp PROPERTIES COMPILE_FLAGS "/Ycpch.h")
endif(MSVC)
