add_library(async SHARED
    detail/fiber.cpp
    detail/hook.cpp
    detail/timer.cpp
    file/socket.cpp
    stream/stream.cpp
    synchronization/futex.cpp
    io_context.cpp
    processor.cpp
    util.cpp
    log.cpp
)

target_link_libraries(async PRIVATE
    pthread
    spdlog::spdlog
    uring
    boost_context
)

target_include_directories(async PUBLIC .)

