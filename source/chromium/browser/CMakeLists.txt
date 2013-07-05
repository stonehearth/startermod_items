FIND_PACKAGE(ChromiumEmbedded REQUIRED)
FIND_PACKAGE(Boost 1.48.0 REQUIRED)

INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${ChromiumEmbedded_INCLUDE_DIRS})

ADD_LIBRARY(chromium_browser STATIC
            response.cpp
            browser.cpp
            pch.cpp
            )

TARGET_LINK_LIBRARIES(chromium_browser
                      csg
                      chromium_app
                      ${ChromiumEmbedded_LIBRARIES}
                      ${ChromiumEmbeddedWrapper_LIBRARIES}
                     )
if (MSVC)
   SET_TARGET_PROPERTIES(chromium_browser PROPERTIES COMPILE_FLAGS "/Yupch.h")
   SET_SOURCE_FILES_PROPERTIES(pch.cpp PROPERTIES COMPILE_FLAGS "/Ycpch.h")
endif(MSVC)
