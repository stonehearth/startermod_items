FIND_PACKAGE(Protobuf REQUIRED)
FIND_PACKAGE(Snappy  REQUIRED)

include_directories(${PROTOBUF_INCLUDE_DIRS})

add_library(dm STATIC
            array.cpp
            boxed.cpp
            boxed_trace.cpp
            boxed_trace_buffered.cpp
            boxed_trace_sync.cpp
            dbg_indenter.cpp
            dbg_info.cpp
            map.cpp
            map_trace.cpp
            map_trace_buffered.cpp
            map_trace_sync.cpp
            object.cpp
            receiver.cpp
            record.cpp
            record_trace.cpp
            record_trace_buffered.cpp
            record_trace_sync.cpp
            set.cpp
            set_trace.cpp
            set_trace_buffered.cpp
            set_trace_sync.cpp
            store.cpp
            store_trace.cpp
            streamer.cpp
            trace.cpp
            tracer.cpp
            tracer_buffered.cpp
            tracer_sync.cpp
            )

target_link_libraries(dm
                      protocols
                      ${Snappy_LIBRARIES}
                      )

# GENERATE_EXPORT_HEADER(dm)
