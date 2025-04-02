include_directories("../src")

add_executable(test_hook test_hook.cpp)
target_link_libraries(test_hook PRIVATE sylar spdlog::spdlog )

