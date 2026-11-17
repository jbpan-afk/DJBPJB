cmake_minimum_required(VERSION 3.8)
project(itl_mean)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(example_interfaces REQUIRED)

# 目标一：算法库。内容与 8.1 的 cpp_lab 完全相同（include/ 放声明，src/ 放定义），
# 节点只用它、不重写它，这样算法和通信是分开的，出错时能分别检查（课件 6.3）。
# 它是纯 C++ 库，不依赖 rclcpp，所以不需要 ament_target_dependencies。
add_library(itl_length_stats src/length_stats.cpp)
target_include_directories(itl_length_stats PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)
target_compile_features(itl_length_stats PUBLIC cxx_std_17)  # std::optional 需要 C++17

# 目标二：发布节点。每秒发布一组长度，几组轮流发，其中一组是空数组。
add_executable(measurement_source src/measurement_source.cpp)
target_compile_features(measurement_source PUBLIC cxx_std_17)
ament_target_dependencies(
  measurement_source
  "rclcpp"
  "example_interfaces"
)

# 目标三：均值节点。订阅长度，调用 meanLength，把均值发布到另一个话题。
# 依赖 itl_length_stats，所以能直接 include "length_stats.hpp"。
add_executable(mean_node src/mean_node.cpp)
target_compile_features(mean_node PUBLIC cxx_std_17)
# 注意：ament_target_dependencies 内部用的是 target_link_libraries 的「无关键字」写法，
# 所以这里链接自己的库时也不能写 PRIVATE，否则 CMake 报两种签名混用。
ament_target_dependencies(
  mean_node
  "rclcpp"
  "example_interfaces"
)
target_link_libraries(mean_node itl_length_stats)

install(TARGETS measurement_source mean_node
  DESTINATION lib/${PROJECT_NAME})

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # comment the line when a copyright and license is added to all source files
  set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # comment the line when this package is in a git repo and when
  # a copyright and license is added to all source files
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
