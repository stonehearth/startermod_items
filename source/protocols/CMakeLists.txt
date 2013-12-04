FIND_PACKAGE(Protobuf REQUIRED)

include_directories(${PROTOBUF_INCLUDE_DIRS})
include_directories(${Boost_INCLUDE_DIR})


PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS
                      radiant.proto
                      store.proto
                      tesseract.proto)

add_library(protobuf_generated_headers STATIC
            ${PROTO_HDRS}
            ${PROTO_SRCS}
            nop.cpp
            )

add_library(protobuf STATIC
            protocol.cpp
            )

add_dependencies(protobuf
                 protobuf_generated_headers)

target_link_libraries(protobuf
                    ${PROTOBUF_LIBRARIES}
                    )
