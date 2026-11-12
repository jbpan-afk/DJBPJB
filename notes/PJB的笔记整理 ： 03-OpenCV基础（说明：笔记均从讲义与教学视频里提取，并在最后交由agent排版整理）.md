# OpenCV 基础：从 `Mat` 到 `minAreaRect`

> 所有 API 都对应本仓库 `src/` 里的真实调用。

## 1. `cv::Mat`：图像在程序里到底是什么

`Mat` 由两部分组成：**矩阵头**（尺寸、类型、指向数据的指针、引用计数）和**像素数据块**。赋值或按值传参默认只复制矩阵头，多个 `Mat` 共享同一块像素；要真正独立副本必须 `clone()` 或 `copyTo()`。

本仓库 `01_pixels.cpp` 在 `assets/armor_red_frame0.png` 上的实测输出：

```text
rows(高) = 819, cols(宽) = 1024
channels = 3
type = 16 , CV_8UC3 = 16
depth = 0 , CV_8U = 0
step[0](每行字节数) = 3072 = cols * channels
像素数据总量 = rows * step = 2515968 字节
```

- `rows` 是高度（行数），`cols` 是宽度（列数）。
- `type = CV_8UC3`：`8U` = 每个通道 8 位无符号（0~255），`C3` = 3 通道。灰度/掩膜是 `CV_8UC1`。
- `step[0]` 是每行字节数，可能大于 `cols * channels`（内存对齐），遍历像素时不能假定它相等。
- 1920×1080 三通道 8 位图像仅像素数据就是 `1920 × 1080 × 3 = 6,220,800` 字节 ≈ 6.2 MB。本图约 2.4 MiB（2,515,968 字节）

**二值掩膜只有 0 和 255 两种取值，但仍按每像素 1 字节存储**，不会因为取值少就自动变成位图——这是讲义 2.2.3 的提醒。

## 2. 坐标与 BGR 顺序

```cpp
cv::Vec3b p = img.at<cv::Vec3b>(y, x);   // 注意先 y（行）后 x（列）
int B = p[0], G = p[1], R = p[2];        // OpenCV 默认彩色图是 BGR，不是 RGB
```

讲义 2.2.1 约定：左上角为原点，x 向右、y 向下，从 0 开始。把通道顺序或行列顺序搞反，程序照样运行，但颜色全错，且很难从变量名上发现——讲义 2.2.4 明确要求「用颜色已知的对象检查实际通道数值」。

本仓库在 0.8 倍缩放的讲义配图上实测（原始坐标 ×0.8 换算后）：

| 讲义原始坐标 | 本图坐标 | B | G | R | S = max(R−B, 0) | 讲义原帧的 S |
| --- | --- | --- | --- | --- | --- | --- |
| (912,662) 灯条边缘 | (730,530) | 42 | 78 | 247 | **205** | 205 |
| (907,662) 灯条中心 | (726,530) | 202 | 253 | 253 | **51** | 42 |
| (914,662) 灯条外侧 | (731,530) | 11 | 20 | 80 | **69** | 35 |
| (900,662) 背景 | (720,530) | 1 | 4 | 14 | **13** | 14 |

两个值得注意的现象：缩放与编码让绝对通道值整体偏移（第一行的 21/57/226 变成 42/78/247），但**颜色差 S 仍是 205**——这正说明「通道差」比「绝对亮度」更稳定；中心像素采样位置在缩放后略有偏移，S 从 42 变成 51，所以讲义 2.3.2 强调"统计只描述给定图像的数值分布"。

## 3. 读写图像

```cpp
cv::Mat img = cv::imread(path, cv::IMREAD_COLOR);   // 强制三通道 BGR
cv::imwrite("results/02_mask.png", mask);
```

`imread` 失败时返回空 `Mat`，必须检查 `img.empty()`，否则后续所有操作都会崩溃。`imshow` 需要图形环境；在无显示/远程会话里一律用 `imwrite` 落盘再看，本仓库全部采用 `imwrite`。

## 4. 通道与算术

```cpp
std::vector<cv::Mat> ch;
cv::split(img, ch);      // ch[0]=B  ch[1]=G  ch[2]=R，各为 CV_8U
cv::merge(ch, out);
cv::cvtColor(bgr, gray, cv::COLOR_BGR2GRAY);
cv::cvtColor(gray, bgr3, cv::COLOR_GRAY2BGR);   // 拼接可视化时统一通道数
```

计算颜色差时尤其注意类型（详见 `01-Cpp基础.md` 第 4 节）：

```cpp
cv::Mat diff16, score16, score8;
cv::subtract(R, B, diff16, cv::noArray(), CV_16S);  // 保留负差
cv::Mat zero = cv::Mat::zeros(diff16.size(), diff16.type());
cv::max(diff16, zero, score16);                     // S = max(R-B, 0)
score16.convertTo(score8, CV_8U);                   // 截断负值后不会超过 255
```

也可以用 `cv::threshold(diff16, score16, 0, 0, cv::THRESH_TOZERO)` 一步得到同样的效果。

## 5. 阈值与掩膜

```cpp
cv::threshold(score8, scoreBin, 60, 255, cv::THRESH_BINARY);  // S > 60 → 255
cv::threshold(R, rBin, 100, 255, cv::THRESH_BINARY);          // R > 100 → 255
cv::bitwise_and(scoreBin, rBin, mask);
```

`THRESH_BINARY` 的判定是**严格大于**阈值，正好符合讲义 2.3.3 的两个「严格大于」。输出是 `CV_8UC1`，前景 255、背景 0。

另一种写法是 `cv::inRange(img, lowerb, upperb, mask)`，适合在 HSV 空间一次框住一个颜色范围；OpenCV 官方教程里 `Thresholding Operations using inRange` 就是讲这个（见培训仓库 `优质资源和教程网址.md` 4.1 节）。

本仓库实测：`S>60` 得到 670 个像素，加上 `R>100` 后剩 623 个（排除 47 个）。讲义原帧是 980 → 948（排除 32 个）。目标通道下限用来排除「相对偏红但本身信号很弱」的像素，讲义特别提醒「在当前主例中作用较小，不代表在其他背景中没有意义」。

## 6. 形态学操作

```cpp
cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
cv::morphologyEx(mask, closed, cv::MORPH_CLOSE, kernel);
```

- **腐蚀**：结构元素覆盖的位置**全部**是前景，中心才保留 → 去掉细窄连接和小点。
- **膨胀**：覆盖范围内**存在**前景，中心就变前景 → 填补缺口、连近邻。
- **开运算** = 先腐蚀后膨胀：去孤立小前景、断细连接。
- **闭运算** = 先膨胀后腐蚀：补小缺口、连邻近区域。

讲义 2.4.2 的关键结论：本例必须用**闭运算**。灯条中心接近白色，`R−B` 最大只有 55，通不过 `S>60`，掩膜是空心细框，两侧常只有 2 像素宽；3×3 腐蚀无法完整落入这些细条，会把左灯条打断成两段，膨胀也恢复不了。讲义中开运算后左灯条变成约 32 和 19 像素两段，`Red_5000` 前 300 帧无候选输出。

形态学只改变像素的空间连通关系，**不知道哪些区域是真目标**。残留的孤立小区域要靠后面的几何筛选排除，而不是继续加形态学。

## 7. 轮廓与最小外接矩形

```cpp
std::vector<std::vector<cv::Point>> contours;
cv::findContours(closed.clone(), contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
//  注意：findContours 会修改输入图像，所以传 clone()
double area = cv::contourArea(c);              // 边界围成的几何面积，≠ 前景像素个数
cv::RotatedRect rr = cv::minAreaRect(c);       // 面积最小的外接矩形，可旋转
```

- `RETR_EXTERNAL` 只取外轮廓，不保留内部孔洞 → 讲义 2.5.1：空心区域和实心区域只要外边界相同，就得到同样的外轮廓，空心灯条因此仍能算出整体几何。
- `CHAIN_APPROX_SIMPLE` 压缩水平/垂直/对角方向的冗余点，只留端点。
- `contourArea` 与前景像素数不同：讲义 2.5.3 的合成灯条有 891 个前景像素，但轮廓面积是 800 平方像素。

### `minAreaRect` 的两个坑（本仓库代码刻意绕开了）

1. `rr.size.width` 和 `rr.size.height` **不保证** width 是长边；不同 OpenCV 版本、不同轮廓，`angle` 的符号和基准都可能变。直接拿 `angle` 当「偏离竖直方向的角度」会错（讲义 2.5.3 明确提醒不要套用原始角度）。
2. 所以 `03_detect.cpp` 从 `rr.points(pts)` 取回四个角点，自己算两条边长，用较长边当长轴：

```cpp
cv::Point2f pts[4]; rr.points(pts);
double len0 = cv::norm(pts[1] - pts[0]);
double len1 = cv::norm(pts[2] - pts[1]);
double L = std::max(len0, len1), W = std::min(len0, len1);
cv::Point2f dir = (len0 >= len1) ? (pts[1]-pts[0]) : (pts[2]-pts[1]);
dir = cv::Point2f(dir.x / cv::norm(dir), dir.y / cv::norm(dir));
if (dir.y < 0) dir = -dir;                        // 统一朝向图像下方
double angle = std::atan2(std::fabs(dir.x), std::fabs(dir.y)) * 180.0 / CV_PI;
cv::Point2f top    = rr.center - cv::Point2f(dir.x*L*0.5, dir.y*L*0.5);
cv::Point2f bottom = rr.center + cv::Point2f(dir.x*L*0.5, dir.y*L*0.5);
```

这就是讲义 2.5.2 的 `p上 = c − (L/2)u`、`p下 = c + (L/2)u`。**长轴端点是上下两端，不是两条长边的中点**——后者连线沿短轴。

## 8. 绘图与结果可视化

```cpp
cv::line(canvas, p1, p2, cv::Scalar(0,255,255), 1);     // Scalar 是 BGR：青色
cv::circle(canvas, p, 3, cv::Scalar(0,0,255), -1);      // -1 表示填充
std::vector<std::vector<cv::Point>> pp{poly};
cv::polylines(canvas, pp, true, cv::Scalar(0,255,0), 2);
cv::putText(canvas, "0", p + cv::Point(8,-8), cv::FONT_HERSHEY_SIMPLEX, 0.8,
            cv::Scalar(255,255,255), 2);
```

`Scalar(B,G,R)`，绿色是 `(0,255,0)`。拼接多张图时先用 `cv::cvtColor(gray, ..., COLOR_GRAY2BGR)` 统一通道数，再 `hconcat` / `vconcat`——`03_stages.png` 就是这么生成的。

## 9. API ↔ 讲义章节对照

| 讲义节 | 内容 | OpenCV API |
| --- | --- | --- |
| 2.2 | 像素、通道、类型、数据量 | `Mat` / `at<Vec3b>` / `split` / `type()` / `step` |
| 2.3 | 颜色差分数、双阈值掩膜 | `subtract` / `max` / `threshold` / `bitwise_and` / `countNonZero` |
| 2.4 | 腐蚀、膨胀、开闭运算 | `getStructuringElement` / `erode` / `dilate` / `morphologyEx` |
| 2.5 | 外轮廓、外接矩形、面积 | `findContours` / `contourArea` / `minAreaRect` / `norm` |
| 2.6 | 配对、凸性、四点顺序 | `Point2f` 向量运算、叉积、鞋带公式 |
| 2.7 | 分层排查 | 日志 + 各阶段 `imwrite` |


---

上一级：[README](../README.md) ｜ 上一篇：[CMake 构建](02-CMake构建.md) ｜ 下一篇：[第一讲知识整理](04-第一讲知识整理.md)
