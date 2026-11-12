#include <opencv2/opencv.hpp>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#ifndef PROJECT_ROOT
#define PROJECT_ROOT "."
#endif

static int regionCount(const cv::Mat& binary, std::vector<double>& areas) {
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(binary.clone(), contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    areas.clear();
    for (const auto& c : contours) areas.push_back(cv::contourArea(c));
    return static_cast<int>(contours.size());
}

int main() {
    const std::string root = PROJECT_ROOT;
    const std::string image_path = root + "/assets/armor_red_frame0.png";
    cv::Mat img = cv::imread(image_path, cv::IMREAD_COLOR);
    if (img.empty()) {
        std::cerr << "无法读取图像: " << image_path << std::endl;
        return 1;
    }

    std::ofstream out(root + "/results/02_mask.log");
    auto log = [&](const std::string& line) {
        std::cout << line << std::endl;
        out << line << std::endl;
    };

    std::vector<cv::Mat> ch;
    cv::split(img, ch);              // ch[0]=B, ch[1]=G, ch[2]=R，均为 CV_8U
    const cv::Mat& B = ch[0];
    const cv::Mat& R = ch[2];

    // S = max(R - B, 0)。用 CV_16S 保存有符号差，避免 uchar 直接相减下溢；
    // 讲义 2.3.2 中的“饱和减法”在此用 max(., 0) 显式实现。
    cv::Mat diff16, score16, score8;
    cv::subtract(R, B, diff16, cv::noArray(), CV_16S);
    cv::Mat zero = cv::Mat::zeros(diff16.size(), diff16.type());
    cv::max(diff16, zero, score16);
    score16.convertTo(score8, CV_8U);   // 差值范围 -255..255，截断负值后不会超过 255

    // 掩膜 M = (S > 60) 且 (R > 100)，两个比较都采用严格大于
    cv::Mat scoreBin, rBin, mask, closed;
    cv::threshold(score8, scoreBin, 60, 255, cv::THRESH_BINARY);
    cv::threshold(R, rBin, 100, 255, cv::THRESH_BINARY);
    cv::bitwise_and(scoreBin, rBin, mask);

    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::morphologyEx(mask, closed, cv::MORPH_CLOSE, kernel);

    cv::imwrite(root + "/results/02_score.png", score8);
    cv::imwrite(root + "/results/02_mask.png", mask);
    cv::imwrite(root + "/results/02_closed.png", closed);

    log("=== 练习 02：颜色差分数、阈值掩膜与形态学 ===");
    log("输入: assets/armor_red_frame0.png  " + std::to_string(img.cols) + "x" + std::to_string(img.rows));
    log("");
    log("[基线参数] 红色分数 S=max(R-B,0)；Tc=60；Tr=100；结构元素 3x3 矩形；闭运算");
    log("");
    const int n_score = cv::countNonZero(scoreBin);
    const int n_r = cv::countNonZero(rBin);
    const int n_mask = cv::countNonZero(mask);
    log("仅 S>60 的前景像素数        = " + std::to_string(n_score));
    log("仅 R>100 的前景像素数       = " + std::to_string(n_r));
    log("两条件取交集后的前景像素数  = " + std::to_string(n_mask));
    log("被 R>100 条件排除的像素数   = " + std::to_string(n_score - n_mask));
    log("讲义主例为 980 -> 948（去掉 32 个）；本图为缩放显示版，数值不同属正常");
    log("");

    std::vector<double> a0, a1;
    const int c0 = regionCount(mask, a0);
    const int c1 = regionCount(closed, a1);
    log("闭运算前外轮廓连通区域数 = " + std::to_string(c0));
    log("闭运算后外轮廓连通区域数 = " + std::to_string(c1));
    log("讲义主例为 9 -> 5：闭运算把左灯条附近碎块并入灯条，远处小区域仍保留，靠面积下限排除");

    std::string small;
    int small_n = 0;
    for (double a : a1) {
        if (a < 20) {
            ++small_n;
            small += " " + std::to_string(a);
        }
    }
    log("闭运算后面积 <20 的区域数 = " + std::to_string(small_n) + "，面积列表:" + small);
    log("");
    log("[输出文件]");
    log("results/02_score.png   颜色差分数图（越亮表示红蓝差异越大）");
    log("results/02_mask.png    双阈值二值掩膜（前景 255 / 背景 0）");
    log("results/02_closed.png  3x3 闭运算结果");
    log("");
    log("[为什么用闭运算而不是开运算]");
    log("灯条中心接近白色，B/G/R 都很大，R-B 不超过 55，无法通过 S>60，掩膜呈空心细框；");
    log("3x3 腐蚀要求结构元素覆盖区域全为前景，会把 2 像素宽的空心边打断，膨胀无法恢复；");
    log("讲义中开运算后左灯条断成约 32 和 19 像素两段，Red_5000 前 300 帧无候选输出。");
    return 0;
}
