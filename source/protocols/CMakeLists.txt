FIND_PACKAGE(Protobuf REQUIRED)

include_directories(${PROTOBUF_INCLUDE_DIRS})
include_directories(${Boost_INCLUDE_DIR})


PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS
                      radiant.proto
                      store.proto
                      tesseract.proto)


add_library(protocols STATIC
            ${PROTO_SRCS}
            ${PROTO_HDRS}
            ${PROTO_PYTHON}
            protocol.cpp
            )

target_link_libraries(protocols
                    ${PROTOBUF_LIBRARIES}
                    )
