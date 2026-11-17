# 第二讲作业：C++ / CMake 与 ROS 2 入门

ITL 战队算法组 2027 秋季培训 · 第二讲《Hello, World!》

本作业围绕一个很小的任务展开：**一组长条长度（单位像素）的均值与计数**。任务本身不需要思考算法，目的是把注意力放在工具链上——数据放在哪（C++）、源码怎么变成程序（CMake）、结果怎么交给另一个程序（ROS 2）。

---

## 目录结构

```
lecture02_homework/
├── .gitignore                   # 排除 build/ install/ log/ 等构建产物（作业 8.3 要求只交源码）
├── cpp_lab/                     # 作业 8.1：纯 C++ + CMake 工程
│   ├── CMakeLists.txt
│   ├── include/length_stats.hpp # 声明（只有一个头文件，别人包含它就能调用）
│   └── src/
│       ├── length_stats.cpp     # 定义（两个函数具体怎么算）
│       └── cpp_lab.cpp          # 带 main 的测试程序
└── ros2_ws/                     # 作业 8.2：ROS 2 工作空间
    └── src/itl_mean/
        ├── CMakeLists.txt
        ├── package.xml
        ├── include/length_stats.hpp   # 与 cpp_lab 中的算法源码完全一致
        ├── src/length_stats.cpp
        ├── src/measurement_source.cpp # 发布节点
        └── src/mean_node.cpp          # 均值节点
```

> **为什么算法有两份？** 8.1 要求是一个独立 CMake 工程，8.2 要求是一个 `ros2 pkg create` 出来的包，两者是两棵独立的构建树，没法直接跨工程 `target_link_libraries`。所以 `itl_mean` 包里放了一份**内容完全相同**的 `length_stats.{hpp,cpp}`，由包内 CMake 编成库再链接给 `mean_node`。节点里一行算法都没有重写——这正是课件 6.3「算法和通信分开」的做法。
>
> 修改算法时请两处同步修改（可用 `diff` 确认）：
> ```bash
> diff cpp_lab/src/length_stats.cpp ros2_ws/src/itl_mean/src/length_stats.cpp
> ```

---

## 一、构建与运行

### 1. C++ 与 CMake（8.1）

```bash
cd cpp_lab
cmake -S . -B build
cmake --build build
./build/cpp_lab
echo $?          # 0 表示全部测试通过；有失败项时是 1
```

不加 `-DCMAKE_BUILD_TYPE` 也能跑；加上 `-DCMAKE_BUILD_TYPE=Release` 更接近实际使用。

### 2. ROS 2（8.2）

环境：Ubuntu 22.04 + ROS 2 **Humble**。

```bash
cd ros2_ws
source /opt/ros/humble/setup.bash
colcon build --symlink-install
source install/setup.bash
```

然后开**两个终端**，每个终端都先执行这三行（`ROS_LOCALHOST_ONLY=1` 是作业要求，避免收到别人节点的数据）：

```bash
cd ros2_ws
source /opt/ros/humble/setup.bash
source install/setup.bash
export ROS_LOCALHOST_ONLY=1
```

- 终端 A 启动均值节点（先启动它，免得错过第一批数据）：
  ```bash
  ros2 run itl_mean mean_node
  ```
- 终端 B 启动发布节点：
  ```bash
  ros2 run itl_mean measurement_source
  ```
- 可选：第三个终端观察均值话题的原始数据
  ```bash
  ros2 topic echo /itl/meanLength
  ```
  按 `Ctrl+C` 结束任意节点。

如果要按本周作业要求提交或做环境隔离，可以先 `export ROS_DOMAIN_ID=xx`（0–101 任选），避免和队里其他人的节点撞在同一域。

---

## 二、节点、话题与消息类型

| 项目 | 值 |
| --- | --- |
| 包名 | `itl_mean` |
| 发布节点名 | `/measurement_source` |
| 发布话题 | `/itl/lengths` |
| 发布消息类型 | `example_interfaces/msg/Float64MultiArray` |
| 均值节点名 | `/mean_node` |
| 订阅话题 | `/itl/lengths` |
| 订阅消息类型 | `example_interfaces/msg/Float64MultiArray` |
| 均值节点发布话题 | `/itl/meanLength` |
| 均值节点消息类型 | `example_interfaces/msg/Float64` |

发布频率：**1 秒一组**，共 4 组，循环发送（第 5 秒回到第 1 组）。发布者的队列深度为 10。

### 每组输入与输出

| 轮次 | 发布到 `/itl/lengths` 的 data | 元素个数 | 均值节点行为 | 是否发布到 `/itl/meanLength` |
| --- | --- | --- | --- | --- |
| 第 1 组 | `[40.0, 60.0, 80.0]` | 3 | 均值 = **60.000** | ✅ 发布 `60.0` |
| 第 2 组 | `[]`（空数组） | 0 | 打印 WARN「收到空数组：本次没有均值，不发布任何消息」 | ❌ **不发布任何消息** |
| 第 3 组 | `[50.5, 61.5]` | 2 | 均值 = **56.000** | ✅ 发布 `56.0` |
| 第 4 组 | `[100.0, 20.0, 30.0]` | 3 | 均值 = **50.000** | ✅ 发布 `50.0` |

实测日志（两个终端同时运行约 12 秒，节选）：

```
# 终端 A：mean_node
[INFO]  [mean_node]: 均值节点就绪：订阅 /itl/lengths，发布 /itl/meanLength
[INFO]  [mean_node]: 收到 3 个长度，均值 = 60.000，发布到 /itl/meanLength
[WARN]  [mean_node]: 收到空数组：本次没有均值，不发布任何消息
[INFO]  [mean_node]: 收到 2 个长度，均值 = 56.000，发布到 /itl/meanLength
[INFO]  [mean_node]: 收到 3 个长度，均值 = 50.000，发布到 /itl/meanLength
[INFO]  [mean_node]: 收到 3 个长度，均值 = 60.000，发布到 /itl/meanLength
[WARN]  [mean_node]: 收到空数组：本次没有均值，不发布任何消息
...

# 终端 B：measurement_source
[INFO]  [measurement_source]: 发布节点就绪：每秒向 /itl/lengths 发布一组长度，共 4 组循环
[INFO]  [measurement_source]: 已发布一组：3 个长度
[INFO]  [measurement_source]: 已发布一组：空数组（0 个长度）
[INFO]  [measurement_source]: 已发布一组：2 个长度
[INFO]  [measurement_source]: 已发布一组：3 个长度
...
```

**验证「空数组真的没发布」**：先启动均值节点、再启动 `ros2 topic echo /itl/meanLength`、最后启动发布节点（顺序很重要，见第五节第 7 条），跑 9 秒。结果是发布节点发出 **9 组**（其中第 2、6 组为空），而 `ros2 topic echo` 只收到 **7 条**，正好是 9 − 2。空数组对应的那两秒，话题上确实一条消息都没有，而不是发了 0 或补发上一次的结果。

自己复现时不必看 9 秒，只要确认「echo 的条数 = 非空组数」即可：

```bash
grep -c '已发布一组' 发布节点日志     # 总组数
grep -c '空数组'     发布节点日志     # 其中空数组的组数
grep -c '^data:'     echo 的日志      # 实际收到的条数，应等于前两者之差
```

`ros2 topic list -t` 的实际输出（后两条是 ROS 2 每个节点都会自动带的系统话题，与本题无关）：

```
/itl/lengths [example_interfaces/msg/Float64MultiArray]
/itl/meanLength [example_interfaces/msg/Float64]
/parameter_events [rcl_interfaces/msg/ParameterEvent]
/rosout [rcl_interfaces/msg/Log]
```

### C++ 测试程序（8.1）的通过情况

```
[PASS] 40/60/80 的均值等于 60
[PASS] 空数组返回 std::nullopt（没有值，而不是 0 或 nan）
[PASS] 单个长度的均值就是它自己
[PASS] 2 和 3 的均值是 2.5（没有发生整数除法）
[PASS] 40/60/80 中不小于 60 的有 2 个（60 算一个）
[PASS] 恰好等于阈值的长度必须被计入（>= 而不是 >）
[PASS] 含两个等于阈值的长度时统计结果为 3
[PASS] 空数组的统计个数是 0
[PASS] 阈值大于所有长度时统计结果为 0
全部测试通过
```

「结果不对时返回非零值」这一条是这样验证的：故意把 `countAtLeast` 里的 `>=` 改成 `>`，重新编译后跑测试，3 项 FAIL、`echo $?` 得到 **1**；改回来后重新变回 0。

---

## 三、两个函数的签名与理由

```cpp
std::optional<double> meanLength(const std::vector<double>& lengths);
std::size_t countAtLeast(const std::vector<double>& lengths, double threshold_px);
```

- **`const std::vector<double>&`**：一帧图像的像素数据可能有几百万个。传引用避免每调用一次就复制一次整份数据；加 `const` 表示函数只看不改，调用者不用担心里面的数据被动手脚。
- **`std::optional<double>`**：返回类型明确声明了「可能没有结果」，调用者必须先 `has_value()` / `if (!mean)` 才能取值，想忽略这种情况都不太容易。
- **`std::size_t`**：个数不可能是负数，用无符号类型表达这个事实。
- **`threshold_px`**：变量名带 `_px` 后缀。类型只管数值不管含义，单位得靠名字写清楚（课件 4.1）。
- **`>=` 而不是 `>`**：题目要求「不小于」，恰好等于阈值的长度必须计入。

---

## 四、为什么空数组的均值不能返回 0（必答题）

这是本讲从头贯穿到尾的问题，课件里分三步给出了答案：

**第一步：不能让它算下去。** `meanLength` 如果直接 `sum / lengths.size()`，空数组就是 `0 / 0`，结果是 `nan`（Not a Number）。`nan` 最坏的地方在于**它不会让程序停下**：它混在后续计算里一路传下去，任何数与它运算仍然是 `nan`。等云台收到一个 `nan` 角度、电机开始乱转的时候，早就追不到它是从哪一帧、哪个空数组产生出来的了。

**第二步：返回 0 同样不行。** 假设返回 0，调用者拿到这个 0，**无法区分两种情况**：
- 灯条长度真的都是 0（均值恰好为 0）；
- 根本没有检测到灯条（没有任何数据）。

这两件事的含义完全相反——前者是「量到了，值为 0」，后者是「什么都没量到」。把 0 当作「没有结果」的标记，就等于让一个合法的数值同时承担两种含义，后面对这个 0 的任何判断都是不可靠的。课件里举的例子很到位：像素 `(0, 0)` 是一个完全合法的位置，所以不能用它当「没检测到目标」的标记。

**第三步：把「没有」明确表达出来。** 换成 `std::optional<double>` 后，「可能没有结果」这件事被写进了**签名**：类型本身就装着「有值」或「空」两种状态，`0` 只能表示「有值，且这个值是 0」。调用方拿到 `optional`，编译器会推着他去先检查再取值。

到了 ROS 2 这一层，这条原则的落点更具体：均值节点收到空数组时，**只写一条 WARN 日志，不发布任何消息**。不发布 0，也不把上一次的结果再发一遍（那等于伪造了一个新数据）。理由很简单：**没有结果，就不要制造一个结果。** 下游订阅者没收到消息，就知道这一帧没有目标；如果收到一个 0 或过期值，它会把一个假数据当成真数据用下去。比赛中「没检测到灯条」是常态，这条路径必须和「检测到了、算出了结果」严格区分开。

---

## 五、遇到的问题与解决

### 1. 为什么算法要分成「头文件 + 源文件」，而不写在一个 .cpp 里

最初把 `meanLength` 直接写在 `cpp_lab.cpp` 里，编译能过。但 ROS 2 的 `mean_node.cpp` 也需要这个函数，如果不拆分，就只能复制一份算法进去——算法有了两个副本，将来改一个忘一个，就会出现「普通程序算对了、节点算错了」这种极难查的问题。

**解决**：按课件 5.2 拆成 `include/length_stats.hpp`（只放声明：函数叫什么、怎么调用）和 `src/length_stats.cpp`（放定义：具体怎么算），编成一个静态库 `itl_cpp_basics`，两个使用方都链接它。理解上要分清：**头文件给编译器看**（检查调用是否正确、在目标文件里留一个"这里需要 meanLength"的记号），**真正的实现是链接时才拼进去的**。

### 2. `std::optional` 报「找不到标识符」

一开始按默认设置编译，`std::optional` / `std::nullopt` 直接不认。原因是它们是 **C++17** 才进标准库的，而 CMake 默认用的标准可能更低。

**解决**：在 `CMakeLists.txt` 里明确要求 C++17：

```cmake
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)   # 编译器不支持 17 就直接报错，而不是悄悄降级
set(CMAKE_CXX_EXTENSIONS OFF)
```

ROS 2 包里对应的是 `target_compile_features(<target> PUBLIC cxx_std_17)`。这里踩过的坑是：**`CMAKE_CXX_STANDARD` 这种全局变量必须在 `add_library` / `add_executable` 之前设置**，否则对已经创建的目标不生效。

### 3. `double a = 2 / 3;` 得到 0.0，程序不报错

课件 4.1 的原例，实际写的时候真踩到了：`sum / lengths.size()` 里如果 `sum` 是 `int`，两个整数相除，小数部分会被直接丢掉。**这类错误最阴险的地方在于它不崩溃、不报警，只是结果不对。**

**解决**：`sum` 声明成 `double`，并且把 `/ static_cast<double>(lengths.size())` 显式写出来。这行类型转换不只是为了正确，也是为了在代码里留下"我知道这里有整数除法的坑"的记号。测试里专门加了 `{2.0, 3.0}` 这一组（期望 2.5），一旦哪天有人把 `sum` 改回 `int`，这条测试立刻红。

### 4. `colcon build` 报 `ModuleNotFoundError: No module named 'catkin_pkg'`

现象：CMake 在解析 `package.xml` 时调用 Python 失败。检查发现 CMake 用的 `python3` 是 `/home/djb/.workbuddy/binaries/python/versions/3.13.12/bin/python3`（PATH 里排在前面的另一个 Python），而 ROS 2 的 `catkin_pkg` / `em` 是装在**系统 Python 3.10** 的 `/usr/lib/python3/dist-packages` 里的。两个 Python 不是同一套环境。

这属于**构建阶段的问题，不是代码问题**——先判断阶段，问题的范围小很多（课件 5.3）。

**解决**：构建时显式指定系统 Python：

```bash
export PATH=/usr/bin:$PATH
colcon build --symlink-install --cmake-args -DPython3_EXECUTABLE=/usr/bin/python3
```

在正常只装了系统 Python 的机器上不需要这一步，直接 `colcon build` 即可。

### 5. `colcon build` 报 `fatal error: error writing to /tmp/xxx.s: 设备上没有空间`

编译时生成汇编中间文件失败，提示写入 `/tmp` 没空间。`df -h /tmp` 看到 `/tmp` 是一个只有 **10 MB** 的 tmpfs，装不下编译中间产物。

**解决**：把编译临时目录指到大磁盘上：

```bash
export TMPDIR=/path/to/某个空间充足的目录
```

同样是环境问题，源码不用改。

### 6. `target_link_libraries` 报「两种签名混用」

给 `mean_node` 链接自己的算法库时写下 `target_link_libraries(mean_node PRIVATE itl_length_stats)`，CMake 直接报错：

```
The plain signature for target_link_libraries has already been used with the target "mean_node".
```

原因是 `ament_target_dependencies()` 内部用的是**无关键字**（plain）写法去调 `target_link_libraries`，而我又用了带 `PRIVATE` 的关键字写法，同一个目标上两种写法不能混用。

**解决**：去掉 `PRIVATE`，保持一致：

```cmake
ament_target_dependencies(mean_node "rclcpp" "example_interfaces")
target_link_libraries(mean_node itl_length_stats)
```

踩过这个坑之后再看「链接时报错先看有没有库没参与链接」，就很具体了。

### 7. 话题明明在，订阅者却收不到消息

第一次跑的时候先启动了发布节点、后启动订阅节点，头几组数据直接丢了，看上去像"程序有问题"。这其实是 ROS 2 的正常行为：**发布者不等订阅者**，没有订阅者时消息就丢了。

**解决**：固定**先启动订阅节点、再启动发布节点**。另外按课件 6.5 的顺序养成排查习惯：① 两个终端是否都 `source` 了同一套环境（`/opt/ros/humble/setup.bash` + `install/setup.bash`）；② 完整话题名是否一致（`/itl/lengths` 和 `itl/lengths` 不是一个话题）；③ 消息类型、QoS 是否相容；④ 订阅者是否被**作为成员长期持有**（写成构造函数的局部变量，构造结束订阅就被取消了）；⑤ 回调是否进来了，只是算错或发布失败（这时看日志就能发现）。

### 8. 一个容易漏掉的细节：订阅者必须是成员变量

`mean_node` 里的 `subscription_` 如果写成构造函数里的局部变量 `auto subscription = create_subscription(...)`，构造函数一结束，`shared_ptr` 引用计数归零，订阅对象被销毁、订阅随之取消。程序**不报任何错**，只是永远收不到消息——正是课件 4.7 讲的"引用和指针不负责让对象继续存在"。所以这里声明成 `rclcpp::Subscription<...>::SharedPtr subscription_` 成员，由节点长期持有。

---

## 六、本讲几条思路在代码里的落点

| 课件里的思路 | 代码里的对应位置 |
| --- | --- |
| 类型只管数值，不管含义，单位要写清楚 | 所有长度变量都带 `_px` 后缀（`length_px`、`threshold_px`） |
| 每段代码都问「输入是空的会怎样」 | `meanLength` 第一行就 `if (lengths.empty()) return std::nullopt;`；`countAtLeast` 空数组自然返回 0 |
| 不用合法数值兼作「没有结果」的标记 | 返回 `std::optional<double>` 而不是 `0` 或 `nan`；节点收到空数组不发布 |
| 看到一个参数，先问副本还是本体、能不能改 | 两个函数都用 `const std::vector<double>&`，不复制也不修改 |
| 报错先判断阶段：编译、链接还是运行 | 见第五节第 4–6 条（链接 / CMake 配置）与第 7 条（运行） |
| 算法和通信分开 | `mean_node.cpp` 里没有任何算法实现，只调用库里的 `meanLength` |
| 节点的形态是登记回调，然后交给 `spin` | 两个节点都是 `main` 里 `rclcpp::spin(...)`，回调分别在订阅和定时器上登记 |

---

## 七、作业要求逐条对照

> 行号按当前提交版本标注。如果你对文件动过手，行号会漂，按函数名 / 变量名找即可。

| 讲义要求 | 落实在哪个文件的哪几行 |
| --- | --- |
| 8.1 声明放在 `include/` 下的头文件 | `cpp_lab/include/length_stats.hpp` 第 11、15 行 |
| 8.1 定义放在 `src/` 下的源文件，编译成一个库 | `cpp_lab/src/length_stats.cpp` 第 9、29 行；`cpp_lab/CMakeLists.txt` 第 10–12 行 |
| 8.1 另写一个带 `main` 的程序链接这个库 | `cpp_lab/src/cpp_lab.cpp`；`cpp_lab/CMakeLists.txt` 第 15–16 行 |
| 8.1 结果不对时返回非零值 | `cpp_lab.cpp` 第 73–77 行（有失败项 `return 1`） |
| 8.1 测试输入包含空数组 | `cpp_lab.cpp` 第 45–47 行（均值）、第 67–68 行（计数） |
| 8.1 测试输入包含恰好等于阈值的长度 | `cpp_lab.cpp` 第 37 行 `exactlyEqual`、第 61–62 行 |
| 8.2 用 `ros2 pkg create` 新建包 | `ros2_ws/src/itl_mean/package.xml`、`CMakeLists.txt` |
| 8.2 发布节点每秒一组、几组轮流、含空数组 | `measurement_source.cpp` 第 16–21 行（四组数据）、第 39 行（1 秒定时器） |
| 8.2 均值节点调用 8.1 的 `meanLength` | `mean_node.cpp` 第 35 行（节点里没有任何算法实现） |
| 8.2 空数组只打印日志、不发布 | `mean_node.cpp` 第 37–43 行 |
| 8.2 消息类型 `Float64MultiArray` / `Float64` | `measurement_source.cpp` 第 34 行、`mean_node.cpp` 第 17、22 行 |
| 8.2 每个终端 `export ROS_LOCALHOST_ONLY=1` | 本文第一节第 2 小节 |
| 8.3 源码不含构建产物 | 仓库根的 `.gitignore`；`build/`、`install/`、`log/` 都不提交 |
| 8.3 README 四要素 | 构建运行命令见第一节；节点名/话题名/消息类型见第二节；为什么不能返回 0 见第四节；问题与解决见第五节 |

---

## 八、现场修改预案

讲义写明「检查时会要求现场修改一条计算规则或一个话题名」。下面这几种改法都试过、能跑通，关键是记住**改完必须重新构建并重新 `source`**，否则运行的还是旧程序（课件 5.1）。

### 1. 改话题名

话题名是两端的唯一约定，所以**发布方和订阅方必须同时改**，一共三处：

| 位置 | 作用 | 改哪一行 |
| --- | --- | --- |
| `measurement_source.cpp` | 发布方发到哪个话题 | 第 35 行 `"/itl/lengths"` |
| `mean_node.cpp` | 订阅方从哪个话题收 | 第 23 行 `"/itl/lengths"`（必须与上面一致） |
| `mean_node.cpp` | 均值发到哪个话题 | 第 18 行 `"/itl/meanLength"` |

只改一端的典型现象：`ros2 topic list` 里两个名字都在，但均值节点永远收不到消息。改完：

```bash
colcon build --symlink-install      # colcon 只重编改动过的包
source install/setup.bash           # 必须重新 source，否则 shell 里还是旧的环境
ros2 topic list -t                  # 确认新话题名已经出现
```

顺手把本文第二节的表格也改掉——文档和代码不一致，检查时会被当成问题。

### 2. 改计算规则

| 想要的改动 | 改哪里 | 连带要改的地方 |
| --- | --- | --- |
| 「不小于」改成「大于」 | `length_stats.cpp` 第 35 行 `>=` → `>`（**两份文件都要改**） | `cpp_lab.cpp` 第 61–62 行的期望值，否则测试会 FAIL——这不是 bug，是测试在如实报告规则已变 |
| 阈值从 60 改成别的 | `cpp_lab.cpp` 第 38 行 `threshold_px` | 第 58–71 行几条断言的期望个数 |
| 改发布频率 1 秒 → 500 毫秒 | `measurement_source.cpp` 第 39 行 `1s` → `500ms` | 无；`500ms` 需要 `using namespace std::chrono_literals`，第 11 行已经有了 |
| 多一组数据 / 换数值 | `measurement_source.cpp` 第 16–21 行 `kMeasurementGroups` | 本文第二节的表格 |
| 求最大值而不是均值 | 头文件加声明、`length_stats.cpp` 加定义、`mean_node.cpp` 改调用与日志 | 返回值不用 `optional` 时要想清楚空数组怎么办——**这正是本讲的核心问题** |

`length_stats.{hpp,cpp}` 有两份（见开头说明），**改算法时两份必须同步**，改完用 `diff` 确认：

```bash
diff cpp_lab/src/length_stats.cpp ros2_ws/src/itl_mean/src/length_stats.cpp && echo "两份一致"
```

### 3. 改完的自检顺序

```bash
# 第一步：先查算法，别看 ROS
cd cpp_lab && cmake --build build && ./build/cpp_lab; echo "exit=$?"

# 第二步：算法对了，再查通信
cd ../ros2_ws && colcon build --symlink-install && source install/setup.bash
ros2 run itl_mean mean_node              # 终端 A
ros2 run itl_mean measurement_source     # 终端 B
ros2 topic echo /itl/meanLength          # 终端 C
```

这就是课件 6.3 说的「算法和通信分开」的实际用法：结果不对时，先在普通程序里用固定输入查算法；算法没问题，再去看消息有没有送到。两类问题不要搅在一起查。

### 4. 30 秒演示「空数组没有发布」

```bash
# 终端 A：先起均值节点，再起 echo（顺序反了会漏掉头几条，见第五节第 7 条）
ros2 run itl_mean mean_node
ros2 topic echo /itl/meanLength

# 终端 B：最后起发布节点，盯着 A 的两个窗口看
ros2 run itl_mean measurement_source
```

A 里会看到 `[INFO] 均值 = 60.000`、`[WARN] 收到空数组…不发布`、`[INFO] 均值 = 56.000` 交替出现；而 echo 窗口里只跟着 INFO 那一行多出一条 `data:`，WARN 那一秒什么都没有。这就是必答题的现场证据。

---

## 九、提交说明

提交内容为**源码**，已删除 `cpp_lab/build/`、`ros2_ws/build/`、`ros2_ws/install/`、`ros2_ws/log/` 等构建产物，仓库根的 `.gitignore` 保证以后也不会误提交。拿到工程后按第一节的命令重新构建即可。

讲义要求作业以 **commit** 的形式提交（次周周六 18:00 后的 commit 视为无效），提交前先确认这几件事：

```bash
git status                 # 只应看到源码与 README、.gitignore，不应出现 build/ install/ log/
git diff --cached --stat   # 确认这次要提交的内容
git commit -m "第二讲作业：C++ 均值/计数库与 ROS 2 发布订阅节点"
git push
```

两个提醒：

- `build/`、`install/`、`log/` 加起来可能有几十 MB，一旦提交进历史就很难彻底删掉。提交前用 `git status` 看一眼，`.gitignore` 生效时它们根本不会出现。
- 提交后**再跑一遍**第一节的命令（从干净状态构建一次），确认拿到仓库的人能直接构建通过。自己机器上能跑、别人 clone 下来跑不起来，最常见的原因就是漏提交了某个文件。

