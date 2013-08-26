add_library(metrics STATIC
            profile.cpp
            store.cpp
            )

target_link_libraries(metrics
                      core
                      metrics
                      resources
                      platform
                      )