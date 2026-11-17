#include "length_stats.hpp"

#include <cmath>
#include <iostream>
#include <optional>
#include <vector>

namespace
{
// 浮点运算可能有极小的误差，不能直接用 == 判断两个 double 是否相等。
bool nearlyEqual(double lhs, double rhs)
{
    return std::fabs(lhs - rhs) < 1e-9;
}

// 一条检查：通过打印 PASS 并返回 0；失败打印 FAIL 并返回 1，交给 main 累加。
int check(bool condition, const char* description)
{
    if (condition)
    {
        std::cout << "[PASS] " << description << '\n';
        return 0;
    }
    std::cerr << "[FAIL] " << description << '\n';
    return 1;
}
}  // namespace

int main()
{
    int failures = 0;

    const std::vector<double> normal{40.0, 60.0, 80.0};         // 课件里的三组数
    const std::vector<double> empty{};                           // 必须测：空数组
    const std::vector<double> withEqual{40.0, 60.0, 60.0, 80.0}; // 含恰好等于阈值
    const std::vector<double> fractional{2.0, 3.0};             // 均值带小数，抓整数除法
    const std::vector<double> exactlyEqual{60.0};                // 恰好等于阈值
    const double threshold_px = 60.0;

    // ---- 测 meanLength ----
    const auto mean = meanLength(normal);
    failures += check(mean.has_value() && nearlyEqual(mean.value(), 60.0),
                      "40/60/80 的均值等于 60");

    const auto noMean = meanLength(empty);
    failures += check(!noMean.has_value(),
                      "空数组返回 std::nullopt（没有值，而不是 0 或 nan）");

    const auto single = meanLength(std::vector<double>{7.5});
    failures += check(single.has_value() && nearlyEqual(single.value(), 7.5),
                      "单个长度的均值就是它自己");

    const auto fracMean = meanLength(fractional);
    failures += check(fracMean.has_value() && nearlyEqual(fracMean.value(), 2.5),
                      "2 和 3 的均值是 2.5（没有发生整数除法）");

    // ---- 测 countAtLeast ----
    failures += check(countAtLeast(normal, threshold_px) == 2,
                      "40/60/80 中不小于 60 的有 2 个（60 算一个）");

    failures += check(countAtLeast(exactlyEqual, threshold_px) == 1,
                      "恰好等于阈值的长度必须被计入（>= 而不是 >）");

    failures += check(countAtLeast(withEqual, threshold_px) == 3,
                      "含两个等于阈值的长度时统计结果为 3");

    failures += check(countAtLeast(empty, threshold_px) == 0,
                      "空数组的统计个数是 0");

    failures += check(countAtLeast(normal, 100.0) == 0,
                      "阈值大于所有长度时统计结果为 0");

    if (failures != 0)
    {
        std::cerr << failures << " 项测试未通过\n";
        return 1;  // 结果不对时返回非零值（作业 8.1 要求）
    }

    std::cout << "全部测试通过\n";
    return 0;  // 0 表示成功，可用 echo $? 查看
}
