include_directories("../src")

add_executable(test_server test_server.cpp)
target_link_libraries(test_server PRIVATE sylar spdlog::spdlog )

add_executable(test_server_mt test_server_mt.cpp)
target_link_libraries(test_server_mt PRIVATE sylar spdlog::spdlog )

