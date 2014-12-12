if (MSVC)
   set(CMAKE_CXX_FLAGS_DEBUG_INIT "/D_DEBUG /MTd /Zi /Od /RTC1")
   set(CMAKE_CXX_FLAGS_MINSIZEREL_INIT     "/MT /O1 /D NDEBUG")
   set(CMAKE_CXX_FLAGS_RELEASE_INIT        "/MT /O2 /Ob2 /Oi /Ot /D NDEBUG")
   set(CMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT "/MT /Zi /O2 /Ob2 /Oi /Ot /D NDEBUG")
endif()
if (NOT DISABLE_WHOLE_PROGRAM_OPTIMIZATION)
   set(CMAKE_CXX_FLAGS_MINSIZEREL_INIT "${CMAKE_CXX_FLAGS_MINSIZEREL_INIT} /GL")
   set(CMAKE_CXX_FLAGS_RELEASE_INIT "${CMAKE_CXX_FLAGS_RELEASE_INIT} /GL")
   set(CMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT "${CMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT} /GL")
endif()
