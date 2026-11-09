cmake_minimum_required(VERSION 3.10)

project(detect_demo)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED)

add_executable(detect_demo main.cpp)

target_link_libraries(detect_demo PRIVATE ${OpenCV_LIBS})

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets/train3.mp4 DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
