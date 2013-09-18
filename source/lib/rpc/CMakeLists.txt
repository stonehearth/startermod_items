add_library(rpc
            pch.cpp
            session.cpp
            function.cpp
            trace.cpp
            untrace.cpp
            core_reactor.cpp
            http_reactor.cpp
            protobuf_reactor.cpp
            lua_router.cpp
            protobuf_router.cpp
            trace_object_router.cpp
            )

if (MSVC)
   set_target_properties(client PROPERTIES COMPILE_FLAGS "/Yupch.h")
   set_source_files_properties(pch.cpp PROPERTIES COMPILE_FLAGS "/Ycpch.h")
endif(MSVC)
