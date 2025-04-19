cmake_minimum_required(VERSION 3.14)
project(ActorCpp VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 添加包含目录
include_directories(include)

# 编译库
add_library(actor_cpp
    src/actor.cpp
    src/event_loop.cpp
    src/message.cpp
    src/scheduler.cpp
)

# 示例程序
add_executable(example_simple examples/simple_example.cpp)
target_link_libraries(example_simple actor_cpp)

# 测试
enable_testing()
add_executable(test_actor tests/test_actor.cpp)
target_link_libraries(test_actor actor_cpp)
add_test(NAME test_actor COMMAND test_actor) 