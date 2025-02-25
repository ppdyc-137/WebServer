include_directories("../src")

add_executable(test_config test_config.cpp)
target_link_libraries(test_config PRIVATE sylar spdlog::spdlog )

add_executable(test_thread test_thread.cpp)
target_link_libraries(test_thread PRIVATE sylar spdlog::spdlog )

add_executable(test_util test_util.cpp)
target_link_libraries(test_util PRIVATE sylar spdlog::spdlog )
