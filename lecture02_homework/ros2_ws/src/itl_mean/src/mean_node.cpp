#include "example_interfaces/msg/float64.hpp"
#include "example_interfaces/msg/float64_multi_array.hpp"
#include "length_stats.hpp"
#include "rclcpp/rclcpp.hpp"

#include <memory>
#include <optional>

// 均值节点：只做三件事——收消息、调用 8.1 的 meanLength、把结果发出去。
// 算法不在这里重写，结果不对时可以先用 cpp_lab 单独查算法（课件 6.3）。
class MeanNode : public rclcpp::Node
{
public:
    MeanNode()
    : Node("mean_node")
    {
        publisher_ = create_publisher<example_interfaces::msg::Float64>(
            "/itl/meanLength", 10);

        // 登记回调：/itl/lengths 每来一条消息，就调用 onMeasurement 一次。
        // 订阅者必须作为成员长期持有，否则构造函数结束订阅就被取消（课件 4.7）。
        subscription_ = create_subscription<example_interfaces::msg::Float64MultiArray>(
            "/itl/lengths", 10,
            [this](const example_interfaces::msg::Float64MultiArray& message) {
                onMeasurement(message);
            });

        RCLCPP_INFO(get_logger(), "均值节点就绪：订阅 /itl/lengths，发布 /itl/meanLength");
    }

private:
    void onMeasurement(const example_interfaces::msg::Float64MultiArray& message)
    {
        // message.data 本身就是 std::vector<double>，按 const& 直接传给算法，不复制。
        const std::optional<double> mean = meanLength(message.data);

        if (!mean.has_value())
        {
            // 空数组：只写一条日志，不发布 0，也不重发上一次的结果。
            // 没有结果，就不要制造一个结果。
            RCLCPP_WARN(get_logger(), "收到空数组：本次没有均值，不发布任何消息");
            return;
        }

        example_interfaces::msg::Float64 result;
        result.data = mean.value();
        RCLCPP_INFO(get_logger(), "收到 %zu 个长度，均值 = %.3f，发布到 /itl/meanLength",
                    message.data.size(), result.data);
        publisher_->publish(result);
    }

    rclcpp::Publisher<example_interfaces::msg::Float64>::SharedPtr publisher_;
    rclcpp::Subscription<example_interfaces::msg::Float64MultiArray>::SharedPtr subscription_;
};

int main(int argc, char* argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<MeanNode>());  // 没有 spin，回调永远不会执行
    rclcpp::shutdown();
    return 0;
}
