#include "length_stats.hpp"

// 头文件里已经包含了这三个，这里再写一遍是「用到什么就自己 include 什么」：
// 不依赖别人替我包含，将来头文件改了，这个文件也不会莫名其妙编译不过。
#include <cstddef>
#include <optional>
#include <vector>

std::optional<double> meanLength(const std::vector<double>& lengths)
{
    // 第一件事永远是先问「输入是空的会怎样」。
    // 不先判断就会算成 0.0 / 0 → nan，错误不会崩溃，而是一路污染到云台。
    if (lengths.empty())
    {
        return std::nullopt;  // 明确表达「没有结果」，不制造一个结果
    }

    // sum 用 double：保证后面的除法是浮点除法，不会像 2/3 那样丢掉小数。
    double sum = 0.0;
    for (const double length_px : lengths)
    {
        sum += length_px;
    }

    // size() 是无符号整数，显式转成 double，把「我知道这里有整数除法坑」写清楚。
    return sum / static_cast<double>(lengths.size());
}

std::size_t countAtLeast(const std::vector<double>& lengths, double threshold_px)
{
    std::size_t count = 0;
    for (const double length_px : lengths)
    {
        // 「不小于」是 >=，包含恰好等于阈值的情况。
        if (length_px >= threshold_px)
        {
            ++count;
        }
    }
    // 空数组会自然走到这里并返回 0：一个元素都没有，本来也数不出东西。
    return count;
}
