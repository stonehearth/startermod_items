add_library(metrics STATIC
            profile.cpp
            store.cpp
            )

target_link_libraries(metrics
                      core
                      math3d_common
                      metrics
                      resources
                      platform
                      )