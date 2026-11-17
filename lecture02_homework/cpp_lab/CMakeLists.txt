cmake_minimum_required(VERSION 3.10)
project(itl_cpp_basics LANGUAGES CXX)

# std::optional / std::nullopt 是 C++17 才进入标准库的，必须明确要求 C++17。
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 目标一：库。声明在 include/ 下的头文件，定义在 src/ 下的源文件。
add_library(itl_cpp_basics src/length_stats.cpp)
target_include_directories(itl_cpp_basics PUBLIC include)
target_compile_options(itl_cpp_basics PRIVATE -Wall -Wextra)

# 目标二：带 main 的测试程序，链接上面的库。
add_executable(cpp_lab src/cpp_lab.cpp)
target_link_libraries(cpp_lab PRIVATE itl_cpp_basics)
target_compile_options(cpp_lab PRIVATE -Wall -Wextra)
