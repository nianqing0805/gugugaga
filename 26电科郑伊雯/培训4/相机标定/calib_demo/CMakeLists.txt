cmake_minimum_required(VERSION 3.16)
project(calib_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenCV REQUIRED)
message(STATUS "OpenCV ${OpenCV_VERSION} at ${OpenCV_DIR}")

add_executable(calib_demo main.cpp)
target_include_directories(calib_demo PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(calib_demo PRIVATE ${OpenCV_LIBS})

if (MSVC)
    target_compile_options(calib_demo PRIVATE /W4 /utf-8)
else()
    target_compile_options(calib_demo PRIVATE -Wall -Wextra)
endif()
