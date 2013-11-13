FIND_PACKAGE(Sfml REQUIRED)

include_directories(${Sfml_INCLUDE_DIRS})

add_library(audio
            input_stream.cpp
            )
