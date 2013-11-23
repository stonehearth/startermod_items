FIND_PACKAGE(Sfml REQUIRED)

include_directories(${Sfml_INCLUDE_DIRS})

add_library(audio
            audio.cpp
            audio_manager.cpp
            input_stream.cpp
            )

