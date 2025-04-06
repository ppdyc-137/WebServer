add_library(sylar SHARED
    detail/fiber.cpp
    detail/hook.cpp
    detail/timer.cpp
    file/socket.cpp
    stream/stream.cpp
    synchronization/futex.cpp
    io_context.cpp
    util.cpp
)

target_link_libraries(sylar PRIVATE
    pthread
    spdlog::spdlog
    uring
    boost_context
)

target_link_libraries(sylar PUBLIC
    yaml-cpp::yaml-cpp
)

target_include_directories(sylar PUBLIC .)

