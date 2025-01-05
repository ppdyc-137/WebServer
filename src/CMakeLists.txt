add_library(sylar SHARED
    config.cpp
)

target_link_libraries(sylar PRIVATE
    spdlog::spdlog
)

target_link_libraries(sylar PUBLIC
    yaml-cpp::yaml-cpp
)


