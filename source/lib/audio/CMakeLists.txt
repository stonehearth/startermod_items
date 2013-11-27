FIND_PACKAGE(Sfml REQUIRED)
FIND_PACKAGE(Libclaw REQUIRED)

include_directories(${Sfml_INCLUDE_DIRS})
include_directories(${Libclaw_INCLUDE_DIRS})

add_library(audio
            audio_manager.cpp
            input_stream.cpp
            channel.cpp
            )

