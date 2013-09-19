FIND_PACKAGE(Protobuf REQUIRED)
FIND_PACKAGE(Snappy  REQUIRED)

include_directories(${PROTOBUF_INCLUDE_DIRS})

add_library(dm STATIC
            guard.cpp
            tracer.cpp
            object.cpp
            record.cpp
            store.cpp
            types.cpp
            pch.cpp
            )

if (MSVC)
   set_target_properties(dm PROPERTIES COMPILE_FLAGS "/Yupch.h")
   set_source_files_properties(pch.cpp PROPERTIES COMPILE_FLAGS "/Ycpch.h")
endif(MSVC)

target_link_libraries(dm
                      protocols
                      ${Snappy_LIBRARIES}
                      )

# GENERATE_EXPORT_HEADER(dm)
