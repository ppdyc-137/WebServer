cmake_minimum_required(VERSION 3.12)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(Bench LANGUAGES C CXX)

find_package(Boost REQUIRED COMPONENTS context system thread)
find_package(Libevent REQUIRED)


add_executable(asio_server bench_asio.cpp)
target_link_libraries(asio_server Boost::context Boost::system Boost::thread)

add_executable(event_server bench_event.cpp)
target_link_libraries(event_server event)

add_executable(asio_async_server bench_asio_async.cpp)
target_link_libraries(asio_async_server Boost::context Boost::system Boost::thread)

add_executable(asio_coro_server bench_asio_coro.cpp)
target_link_libraries(asio_coro_server Boost::context Boost::system Boost::thread)
