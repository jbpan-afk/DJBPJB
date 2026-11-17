#ifndef ITL_LENGTH_STATS_HPP_
#define ITL_LENGTH_STATS_HPP_

#include <cstddef>
#include <optional>
#include <vector>

// 求一组灯条长度（单位：像素）的均值。
// 空数组没有均值：返回 std::nullopt，而不是 0，也不是 nan。
// 参数写成 const&：数据既不复制，也不允许函数修改（课件 4.3 / 4.4）。
std::optional<double> meanLength(const std::vector<double>& lengths);

// 统计一组长度中「不小于 threshold_px」的个数，恰好等于阈值也算一个。
// 返回 std::size_t：个数不会是负数。
std::size_t countAtLeast(const std::vector<double>& lengths, double threshold_px);

#endif  // ITL_LENGTH_STATS_HPP_
