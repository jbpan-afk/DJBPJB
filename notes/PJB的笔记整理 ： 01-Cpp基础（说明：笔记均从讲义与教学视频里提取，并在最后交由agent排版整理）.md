# C++ ：OpenCV 代码

## 1. 从 `.cpp` 到可执行文件

一次 C++ 构建分四步：

```text
预处理（展开 #include、宏） → 编译（生成汇编） → 汇编（生成 .o） → 链接（合并库，生成可执行文件）
```

单文件程序可以直接手写命令：

```bash
g++ -std=c++17 -O2 src/01_pixels.cpp -o demo $(pkg-config --cflags --libs opencv4)
```

`pkg-config --cflags` 给出头文件路径，`--libs` 给出要链接的库列表。漏掉后者会得到 `undefined reference to cv::imread(...)`——这是**链接错误**，不是语法错误。

## 2. 头文件与源文件、声明与定义

- `#include <opencv2/opencv.hpp>`：尖括号表示去系统/编译器指定的路径找；`#include "my.h"` 双引号先找当前目录。
- 头文件放**声明**（这个函数长什么样），源文件放**定义**（函数体怎么写）。
- OpenCV 的头里已经写好了 `cv::imread` 的声明；真正的实现在 `.so` 动态库里，靠链接这一步接上。
- 自己写头文件时加 `#pragma once`，避免被重复包含。

## 3. 命名空间

`cv::Mat`、`cv::Point2f`、`cv::imread` 里的 `cv::` 是命名空间，防止不同库的函数重名。本仓库代码一律写全 `cv::`，不写 `using namespace cv;`——在大型工程里全局 `using` 容易引入名字冲突，读代码时也看不出函数来自哪个库。同理 `std::` 也写全。

## 4. 类型、变量与 8 位溢出

| 类型 | 含义 | 本仓库用途 |
| --- | --- | --- |
| `int` / `double` / `bool` | 整数 / 双精度浮点 / 布尔 | 计数、几何量、判定 |
| `auto` | 让编译器推导类型 | `const auto& c = contours[i];` |
| `std::string` | 字符串 | 路径、日志文本 |
| `uchar`（`unsigned char`） | 8 位无符号，0~255 | 单个通道值 |
| `cv::Vec3b` | 3 个 `uchar` | 一个 BGR 像素 |
| `cv::Point2f` | 两个 `float` | 亚像素精度的图像点 |
| `cv::Mat` | 图像矩阵 | 图像、掩膜 |

讲义 2.3.2 的分数是 `S = max(R - B, 0)`。通道是 `uchar`，如果直接写 `R - B`，当 `R=20, B=200` 时结果 `-180` 会绕回成一个正数（无符号下溢），掩膜完全错。我给出正确做法：

```cpp
// 做法 A（本仓库 02_mask.cpp 采用）：用 16 位有符号矩阵保存差值
cv::Mat diff16;
cv::subtract(R, B, diff16, cv::noArray(), CV_16S);   // CV_16S = 16 位有符号整数
cv::Mat zero = cv::Mat::zeros(diff16.size(), diff16.type());
cv::max(diff16, zero, score16);                       // 等价于 max(R-B, 0)

// 做法 B：单像素计算时先转成 int
int S = std::max(int(p[2]) - int(p[0]), 0);
```

讲义里说的「饱和减法」指的就是把负结果钳到 0，OpenCV 的 `cv::subtract` 在 `CV_8U` 输出时会自动做饱和；这里因为要保留有符号差再取 max，所以显式用了 `CV_16S`

## 5. `const` 与引用 `&`

```cpp
const cv::Mat& R = ch[2];                 // 引用：不复制整个图像，只是别名
void foo(const std::vector<cv::Point>& c); // const 引用：只读、不拷贝
```

`cv::Mat` 本身是「头 + 引用计数数据块」的结构，按值传参不会立刻复制像素（浅拷贝），但语义上仍建议用 `const &` 表达只读意图；需要真正独立副本时用 `clone()`。

## 6. `std::vector`：轮廓点集的嵌套类型

`findContours` 的输出类型是：

```cpp
std::vector<std::vector<cv::Point>> contours;
// 外层：一条条轮廓；内层：该轮廓上的边界点
```

常用操作：`size()`（元素个数）、`push_back()`（追加）、`clear()`、范围 for：

```cpp
for (const auto& c : contours) {
    double area = cv::contourArea(c);
}
```

`03_detect.cpp` 里 `bars` 是 `std::vector<LightBar>`，配对时用两层 `for` 枚举 `i < j`，避免自己和自己配、也避免重复组合。

## 7. `struct` / 类：把一组数据打包

讲义 2.5 描述了灯条的长度、宽度、中心、长轴方向、上下端点。C++ 里用结构体把这些量放在一起，比散落的变量清晰：

```cpp
struct LightBar {
    cv::Point2f center, dir, top, bottom;
    double length = 0, width = 0, angle = 0, area = 0;
    std::vector<cv::Point2f> box;
};
```

这是本仓库 `src/03_detect.cpp` 里的真实定义。`struct` 成员默认公开；需要封装行为时用 `class` 并加成员函数。

## 8. 函数与 lambda

本仓库日志用了一个 lambda（匿名函数），同时输出到终端和文件：

```cpp
std::ofstream out(root + "/results/03_detect.log");
auto log = [&](const std::string& line) {
    std::cout << line << std::endl;
    out << line << std::endl;
};
log("通过筛选的灯条数 = " + std::to_string(bars.size()));
```

`[&]` 表示按引用捕获外部变量（`out`）。数值转字符串用 `std::to_string`；要控制小数位用 `<sstream>` 里的 `std::ostringstream` + `std::setprecision`（本仓库的 `fmt()` 函数）。

## 9. C++17 相关的几处

`CMakeLists.txt` 里设了 `CMAKE_CXX_STANDARD 17`。本仓库用到的 C++17 / 现代写法：`auto` 类型推导、范围 for、列表初始化 `std::vector<cv::Point2f> quad{...}`、`std::clamp`（`03_detect.cpp` 里用 `std::max`/`std::min` 实现了同样的点积钳位）、`std::string` 的 `+` 拼接。

## 10. 常见报错对照

| 报错 | 原因 | 处理 |
| --- | --- | --- |
| `fatal error: opencv2/opencv.hpp: No such file` | 缺头文件路径 | CMake 里 `find_package(OpenCV REQUIRED)` + `target_include_directories` |
| `undefined reference to cv::...` | 链接时没带 OpenCV 库 | `target_link_libraries(... ${OpenCV_LIBS})` |
| `'Mat' was not declared` | 少了 `cv::` 或头文件 | 补全命名空间 |
| `conversion from ... to ...` | 类型不匹配 | 显式 `static_cast` 或构造 `cv::Point2f(float, float)` |
| `error: 'clamp' is not a member of std` | 标准设成了 C++14 | 检查 `set(CMAKE_CXX_STANDARD 17)` |


---

上一级：[README](../README.md) ｜ 下一篇：[CMake 构建](02-CMake构建.md)
