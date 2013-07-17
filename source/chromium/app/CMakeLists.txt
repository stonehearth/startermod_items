FIND_PACKAGE(ChromiumEmbedded REQUIRED)

INCLUDE_DIRECTORIES(${ChromiumEmbedded_INCLUDE_DIRS})

ADD_LIBRARY(chromium_app STATIC
            app.cpp
            )

TARGET_LINK_LIBRARIES(chromium_app
                      ${ChromiumEmbeddedWrapper_LIBRARIES}
                     )
                      