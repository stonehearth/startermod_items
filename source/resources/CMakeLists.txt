FIND_PACKAGE(Cryptopp REQUIRED)
FIND_PACKAGE(Poco REQUIRED)
FIND_PACKAGE(FlatBuffers REQUIRED)

include_directories(${Cryptopp_INCLUDE_DIRS})
include_directories(${Poco_INCLUDE_DIRS})
include_directories(${FLATBUFFERS_INCLUDE_DIR})
include_directories(${CMAKE_BINARY_DIR}/source/resources)

FLATBUFFERS_GENERATE_C_HEADERS(FBS
                               lua_file_mapper.fbs)

add_library(resources STATIC
            ${FBS_OUTPUTS}
            animation.cpp
            manifest.cpp
            directory_module.cpp
            zip_module.cpp
            res_manager.cpp
            lua_file_mapper.cpp
            resource_compiler.cpp
            )

target_link_libraries(resources
                      ${Poco_LIBRARIES}
                     )
