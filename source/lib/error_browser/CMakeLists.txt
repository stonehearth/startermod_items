FIND_PACKAGE(Boost 1.48.0 COMPONENTS system regex date_time program_options REQUIRED)
FIND_PACKAGE(Gflags     REQUIRED)
FIND_PACKAGE(Glog       REQUIRED)
FIND_PACKAGE(Libjson REQUIRED)

include_directories(${Boost_INCLUDE_DIR})
include_directories(${Libjson_INCLUDE_DIRS})

add_library(error_browser
            error_browser.cpp
            error_browser_record.cpp
            pch.cpp
            )

if (MSVC)
   set_target_properties(client PROPERTIES COMPILE_FLAGS "/Yupch.h")
   set_source_files_properties(pch.cpp PROPERTIES COMPILE_FLAGS "/Ycpch.h")
endif(MSVC)

target_link_libraries(error_browser
                      om
                     )
