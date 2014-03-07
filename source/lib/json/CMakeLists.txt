add_library(lib_json
            node.cpp
            csg_json.cpp
            dm_json.cpp
            core_json.cpp
            json.cpp
            )

add_dependencies(lib_json
                 om_generated_headers)
