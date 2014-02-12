FIND_PACKAGE(ChromiumEmbedded REQUIRED)

INCLUDE_DIRECTORIES(${ChromiumEmbedded_INCLUDE_DIRS})

ADD_LIBRARY(chromium_browser STATIC
            response.cpp
            browser.cpp
            )

TARGET_LINK_LIBRARIES(chromium_browser
                      csg
                      chromium_app
                      ${ChromiumEmbedded_LIBRARIES}
                      ${ChromiumEmbeddedWrapper_LIBRARIES}
                     )
