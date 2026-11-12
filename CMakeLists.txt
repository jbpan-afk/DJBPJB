cmake_minimum_required(VERSION 3.10)
project(itl_l1_homework LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenCV REQUIRED)

# 把仓库根目录传给源码，使程序从任意工作目录运行都能找到 assets/ 与 results/
add_executable(01_pixels src/01_pixels.cpp)
target_include_directories(01_pixels PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(01_pixels PRIVATE ${OpenCV_LIBS})
target_compile_definitions(01_pixels PRIVATE PROJECT_ROOT="${CMAKE_CURRENT_SOURCE_DIR}")

add_executable(02_mask src/02_mask.cpp)
target_include_directories(02_mask PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(02_mask PRIVATE ${OpenCV_LIBS})
target_compile_definitions(02_mask PRIVATE PROJECT_ROOT="${CMAKE_CURRENT_SOURCE_DIR}")

add_executable(03_detect src/03_detect.cpp)
target_include_directories(03_detect PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(03_detect PRIVATE ${OpenCV_LIBS})
target_compile_definitions(03_detect PRIVATE PROJECT_ROOT="${CMAKE_CURRENT_SOURCE_DIR}")
