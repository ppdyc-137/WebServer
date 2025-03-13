include_directories("../src")

add_executable(test_config test_config.cpp)
target_link_libraries(test_config PRIVATE sylar spdlog::spdlog )

add_executable(test_thread test_thread.cpp)
target_link_libraries(test_thread PRIVATE sylar spdlog::spdlog )

add_executable(test_util test_util.cpp)
target_link_libraries(test_util PRIVATE sylar spdlog::spdlog )

add_executable(test_fiber test_fiber.cpp)
target_link_libraries(test_fiber PRIVATE sylar spdlog::spdlog )

add_executable(test_scheduler test_scheduler.cpp)
target_link_libraries(test_scheduler PRIVATE sylar spdlog::spdlog )

add_executable(test_iomanager test_iomanager.cpp)
target_link_libraries(test_iomanager PRIVATE sylar spdlog::spdlog )

add_executable(test_hook test_hook.cpp)
target_link_libraries(test_hook PRIVATE sylar spdlog::spdlog )

add_executable(test_address test_address.cpp)
target_link_libraries(test_address PRIVATE sylar spdlog::spdlog )
