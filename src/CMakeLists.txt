add_library(sylar SHARED
    detail/fiber.cpp
    detail/hook.cpp
    detail/timer.cpp
    file/socket.cpp
    stream/stream.cpp
    synchronization/futex.cpp
    io_context.cpp
    processor.cpp
    util.cpp
)

target_link_libraries(sylar PRIVATE
    pthread
    spdlog::spdlog
    uring
    boost_context
)

target_include_directories(sylar PUBLIC .)

