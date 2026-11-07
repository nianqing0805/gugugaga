cmake_minimum_required(VERSION 3.10)

project(hsv_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 不要写 find_package(OpenCV 4 REQUIRED)，环境是 5.x 时会找不到
find_package(OpenCV REQUIRED)

add_executable(hsv_demo main.cpp)

target_include_directories(hsv_demo PRIVATE ${OpenCV_INCLUDE_DIRS})
target_include_directories(hsv_demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../detect_demo/src)
target_link_libraries(hsv_demo PRIVATE ${OpenCV_LIBS})

# 把测试图片拷到构建目录，这样在 build/ 里运行也能读到 armor.jpg
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets/armor.jpg DESTINATION ${CMAKE_CURRENT_BINARY_DIR})