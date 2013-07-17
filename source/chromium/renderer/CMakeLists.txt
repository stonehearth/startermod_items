FIND_PACKAGE(ChromiumEmbedded REQUIRED)

INCLUDE_DIRECTORIES(${ChromiumEmbedded_INCLUDE_DIRS})

ADD_EXECUTABLE(chromium_renderer
               WIN32
               main_win32.cpp
               )

TARGET_LINK_LIBRARIES(chromium_renderer
                      chromium_app
                      ${ChromiumEmbedded_LIBRARIES}
                      )
