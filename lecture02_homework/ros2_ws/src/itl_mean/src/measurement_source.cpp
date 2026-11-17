#include "example_interfaces/msg/float64_multi_array.hpp"
#include "rclcpp/rclcpp.hpp"

#include <chrono>
#include <cstddef>
#include <memory>
#include <vector>

namespace
{
using namespace std::chrono_literals;  // 让 1s 这种写法可用

// 几组模拟的灯条长度（单位：像素），轮流发布。
// 第一组就是课件里的 40、60、80；其中必须有一组是空数组，
// 用来验证下游「没有结果时不要制造结果」的处理（课件 6.3）。
const std::vector<std::vector<double>> kMeasurementGroups{
    {40.0, 60.0, 80.0},   // 均值 60
    {},                   // 空数组：没有检测到灯条，这是比赛中的常态
    {50.5, 61.5},         // 均值 56，小数情况
    {100.0, 20.0, 30.0},  // 均值 50
};
}  // namespace

// 发布节点：只负责「产生数据并放到话题上」，不关心谁在听，
// 也不关心对方拿到后怎么算。这是算法与通信分开的一半（课件 6.3）。
class MeasurementSource : public rclcpp::Node
{
public:
    MeasurementSource()
    : Node("measurement_source")
    {
        // 话题名、消息类型是发布者和订阅者之间唯一的约定。
        // 队列长度 10：订阅者偶尔慢一点，最多缓存 10 条消息。
        publisher_ = create_publisher<example_interfaces::msg::Float64MultiArray>(
            "/itl/lengths", 10);

        // 定时器：每隔 1 秒调用一次 publishNextGroup，
        // 和「收到消息就调用回调」一样，也是登记回调、交给 spin（课件 6.4）。
        timer_ = create_wall_timer(1s, [this]() { publishNextGroup(); });

        RCLCPP_INFO(get_logger(),
                    "发布节点就绪：每秒向 /itl/lengths 发布一组长度，共 %zu 组循环",
                    kMeasurementGroups.size());
    }

private:
    void publishNextGroup()
    {
        // 取当前这一组，然后把下标往后挪一格；到末尾就回到第一组。
        const std::vector<double>& group = kMeasurementGroups[index_];
        index_ = (index_ + 1) % kMeasurementGroups.size();

        example_interfaces::msg::Float64MultiArray message;
        // data 的类型就是 std::vector<double>，可以直接赋值，不用逐个 push_back。
        message.data = group;
        publisher_->publish(message);

        if (group.empty())
        {
            RCLCPP_INFO(get_logger(), "已发布一组：空数组（0 个长度）");
        }
        else
        {
            RCLCPP_INFO(get_logger(), "已发布一组：%zu 个长度", group.size());
        }
    }

    rclcpp::Publisher<example_interfaces::msg::Float64MultiArray>::SharedPtr publisher_;
    rclcpp::TimerBase::SharedPtr timer_;
    std::size_t index_ = 0;  // 当前发到第几组
};

int main(int argc, char** argv)
{
    rclcpp::init(argc, argv);
    // 把控制权交给 spin：之后由它按时间调用定时器回调，直到按下 Ctrl+C。
    rclcpp::spin(std::make_shared<MeasurementSource>());
    rclcpp::shutdown();
    return 0;
}
