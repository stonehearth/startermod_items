FIND_PACKAGE(Protobuf REQUIRED)
FIND_PACKAGE(Snappy  REQUIRED)

include_directories(${PROTOBUF_INCLUDE_DIRS})

add_library(dm STATIC
            alloc_trace.cpp
            dbg_info.cpp
            dbg_indenter.cpp
            object.cpp
            record.cpp
            store.cpp
            receiver.cpp
            record_loader.cpp
            streamer.cpp
            trace.cpp
            tracer.cpp
            tracer_sync.cpp
            tracer_buffered.cpp
            )

target_link_libraries(dm
                      protocols
                      ${Snappy_LIBRARIES}
                      )

# GENERATE_EXPORT_HEADER(dm)
