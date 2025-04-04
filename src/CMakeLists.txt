add_library(sylar SHARED
    util.cpp
    fiber.cpp
    io_context.cpp
    hook.cpp
    timer.cpp
    socket.cpp
    stream/stream.cpp
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

