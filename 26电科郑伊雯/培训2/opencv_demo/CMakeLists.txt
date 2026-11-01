cmake_minimum_required(VERSION 3.11)

project(OPENCV_DEMO)

set(CMAKE_CXX_STANDARD 11)

find_package(OpenCV REQUIRED)

include_directiories(${OpenCV_INCLUDE_DIRS})

add_executable(main main.cpp)

target_link_libraries(main ${OpenCV_LIBS})