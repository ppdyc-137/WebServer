add_library(sylar SHARED
    config.cpp
    thread.cpp
)

target_link_libraries(sylar PRIVATE
    pthread
    spdlog::spdlog
)

target_link_libraries(sylar PUBLIC
    yaml-cpp::yaml-cpp
)


