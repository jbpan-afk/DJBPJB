# CMake：把三个练习组织成一个可复现的工程

## 1. 为什么不能一直手写 `g++`

一个真实工程会有多个源文件、多个第三方库（OpenCV、ROS 2、推理后端）、不同的编译选项和 debug/release 配置。手写命令会遇到三个问题：库路径每台机器不同、文件增删要改命令、构建产物和源码混在一起。CMake 是**构建系统的生成器**：读 `CMakeLists.txt`，生成 Makefile 或 Ninja 文件，再由 `cmake --build` 调用编译器。

CMake 本身不编译代码，g++ 才编译。

## 2. 本仓库 `CMakeLists.txt` 逐行解释

```cmake
cmake_minimum_required(VERSION 3.10)          # 要求的最低 CMake 版本
project(itl_l1_homework LANGUAGES CXX)        # 工程名；只启用 C++

set(CMAKE_CXX_STANDARD 17)                    # 讲义 4.1.1 要求 C++17
set(CMAKE_CXX_STANDARD_REQUIRED ON)           # 编译器不支持 17 就报错，而不是悄悄降级
set(CMAKE_CXX_EXTENSIONS OFF)                 # 用标准 C++，不用 GNU 扩展

find_package(OpenCV REQUIRED)                 # 找 OpenCV，找不到就配置失败

add_executable(01_pixels src/01_pixels.cpp)   # 由这个源文件生成一个可执行目标
target_include_directories(01_pixels PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(01_pixels PRIVATE ${OpenCV_LIBS})
target_compile_definitions(01_pixels PRIVATE PROJECT_ROOT="${CMAKE_CURRENT_SOURCE_DIR}")
```

三个目标 `01_pixels` / `02_mask` / `03_detect` 结构相同，各自生成一个可执行文件。

### `find_package(OpenCV REQUIRED)` 做了什么

在系统路径里找 `OpenCVConfig.cmake`，成功后提供两个变量：

- `OpenCV_INCLUDE_DIRS`：头文件目录，本机是 `/usr/include/opencv4`（所以 `#include <opencv2/opencv.hpp>` 能找到）。
- `OpenCV_LIBS`：需要链接的库列表（`opencv_core`、`opencv_imgproc`、`opencv_imgcodecs` 等）。

本机配置时的真实输出：

```text
-- The CXX compiler identification is GNU 11.4.0
-- Found OpenCV: /usr (found version "4.5.4")
-- Build files have been written to: <仓库目录>/build
```

`<仓库目录>` 指 clone 下来的仓库根路径，每台机器各不相同。

`REQUIRED` 表示找不到就立刻失败，不要等到链接阶段才报一堆 `undefined reference`。

### `PRIVATE` 是什么意思

`target_*` 系列命令把属性挂在**具体目标**上，而不是全局。`PRIVATE` 表示这个头文件路径/库只给 `01_pixels` 自己用，不会传递给依赖它的其他目标。工程变大后，`PUBLIC` / `INTERFACE` 才需要区分；现在统一写 `PRIVATE` 即可。

### `PROJECT_ROOT` 宏

```cpp
#ifndef PROJECT_ROOT
#define PROJECT_ROOT "."
#endif
const std::string root = PROJECT_ROOT;
cv::imread(root + "/assets/armor_red_frame0.png");
```

CMake 把仓库根目录的绝对路径编译进程序，所以无论从哪个工作目录执行 `./build/03_detect`，都能正确找到 `assets/` 和 `results/`。这是「运行结果可复现」的一个小但关键的设计。

## 3. 构建与运行（本机实测通过）

```bash
cd ~/RM_CV_Homework   # 换成你自己的仓库目录
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release   # 配置：生成 build/ 下的构建系统
cmake --build build -j4                          # 编译：等价于 make -C build -j4
./build/01_pixels
./build/02_mask
./build/03_detect
```

`-S` 是源码目录，`-B` 是构建目录，这种叫**源码外构建（out-of-source build）**：所有中间产物都在 `build/`，源码目录保持干净。

`.gitignore` 里写了 `build/`，理由是第零讲讲过的：这些文件都能由源码重新生成，提交进仓库只会制造无意义的 diff 和冲突。首次 clone 仓库的人只要重跑上面两条 cmake 命令即可。

## 4. 什么时候需要重新配置

- 改了 `CMakeLists.txt`：直接 `cmake --build build` 通常会自动触发重新配置。
- 新增/删除源文件、换编译器、换 OpenCV 版本：删掉 `build/` 重新跑 `cmake -S . -B build` 最干净（`build/CMakeCache.txt` 会缓存旧路径，有时会「记住」错误配置）。
- 系统有多个 OpenCV 版本时，用 `cmake -S . -B build -DOpenCV_DIR=/path/to/opencv` 或设置 `CMAKE_PREFIX_PATH` 指定。

## 5. 常见报错

| 现象 | 原因 | 处理 |
| --- | --- | --- |
| `Could not find a package configuration file provided by "OpenCV"` | 没装开发包 | `sudo apt install libopencv-dev`，用 `pkg-config --modversion opencv4` 确认（本机 4.5.4） |
| `fatal error: opencv2/opencv.hpp: No such file` | 漏了 `target_include_directories` | 补上 `${OpenCV_INCLUDE_DIRS}` |
| `undefined reference` | 漏了 `target_link_libraries` | 补上 `${OpenCV_LIBS}` |
| 改了 CMakeLists 没生效 | 缓存未刷新 | 删除 `build/` 重新配置 |
| 头文件在 `opencv4/opencv2/...` 下 | Ubuntu 的包布局 | 由 `OpenCV_INCLUDE_DIRS` 处理，不要手写路径 |

## 6. 再往前一步

- 源文件变多时，用 `file(GLOB ...)` 或显式列出，把公共代码抽成 `add_library(detector STATIC ...)`，三个可执行目标链接同一个库。
- 编译选项：`target_compile_options(tgt PRIVATE -Wall -Wextra)` 能在编译期发现很多低级错误。
- 引入 ROS 2 后，构建系统换成 `ament_cmake`，用 `colcon build`；`find_package(ament_cmake REQUIRED)`、`find_package(rclcpp REQUIRED)`、`ament_target_dependencies(...)` 的形式和这里完全一致，只是包名换了。


---

上一级：[README](../README.md) ｜ 上一篇：[C++ 基础](01-Cpp基础.md) ｜ 下一篇：[OpenCV 基础](03-OpenCV基础.md)
