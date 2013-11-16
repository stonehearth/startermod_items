FIND_PACKAGE(Protobuf REQUIRED)
FIND_PACKAGE(Snappy  REQUIRED)

include_directories(${PROTOBUF_INCLUDE_DIRS})

add_library(dm STATIC
            dbg_info.cpp
            dbg_indenter.cpp
            object.cpp
            record.cpp
            store.cpp
            receiver.cpp
            streamer.cpp
            tracer_buffered.cpp
            )

target_link_libraries(dm
                      protocols
                      ${Snappy_LIBRARIES}
                      )

# GENERATE_EXPORT_HEADER(dm)
