add_library(sylar SHARED
    config.cpp
    util.cpp
    thread.cpp
    mutex.cpp
    fiber.cpp
    log.cpp
    scheduler.cpp
    iomanager.cpp
    timer.cpp
)

target_link_libraries(sylar PRIVATE
    pthread
    spdlog::spdlog
)

target_link_libraries(sylar PUBLIC
    yaml-cpp::yaml-cpp
)


