if (MSVC)
   set(CMAKE_C_FLAGS_DEBUG_INIT "/D_DEBUG /MTd /Zi /Od /RTC1")
   set(CMAKE_C_FLAGS_MINSIZEREL_INIT     "/MT /O1 /GL /D NDEBUG")
   set(CMAKE_C_FLAGS_RELEASE_INIT        "/MT /O2 /Ob2 /Oi /Ot /GL /D NDEBUG")
   set(CMAKE_C_FLAGS_RELWITHDEBINFO_INIT "/MT /Zi /O2 /Ob2 /Oi /Ot /GL /D NDEBUG")
endif()
