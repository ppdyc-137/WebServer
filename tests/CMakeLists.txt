include_directories("../src")

add_executable(test_hook test_hook.cpp)
target_link_libraries(test_hook PRIVATE sylar spdlog::spdlog )

add_executable(test_server test_server.cpp)
target_link_libraries(test_server PRIVATE sylar spdlog::spdlog )

add_executable(test_server_mt test_server_mt.cpp)
target_link_libraries(test_server_mt PRIVATE sylar spdlog::spdlog )

add_executable(test_timer test_timer.cpp)
target_link_libraries(test_timer PRIVATE sylar spdlog::spdlog )

add_executable(test_file test_file.cpp)
target_link_libraries(test_file PRIVATE sylar spdlog::spdlog )

add_executable(test_stream test_stream.cpp)
target_link_libraries(test_stream PRIVATE sylar spdlog::spdlog )

add_executable(test_timeout test_timeout.cpp)
target_link_libraries(test_timeout PRIVATE sylar spdlog::spdlog )

add_executable(test_mutex test_mutex.cpp)
target_link_libraries(test_mutex PRIVATE sylar spdlog::spdlog )
