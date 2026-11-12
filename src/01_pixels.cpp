#include <opencv2/opencv.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>

#ifndef PROJECT_ROOT
#define PROJECT_ROOT "."
#endif

int main() {
    const std::string root = PROJECT_ROOT;
    const std::string image_path = root + "/assets/armor_red_frame0.png";
    cv::Mat img = cv::imread(image_path, cv::IMREAD_COLOR);
    if (img.empty()) {
        std::cerr << "无法读取图像: " << image_path << std::endl;
        return 1;
    }

    std::ofstream out(root + "/results/01_pixels.txt");
    auto log = [&](const std::string& line) {
        std::cout << line << std::endl;
        out << line << std::endl;
    };

    log("=== 练习 01：像素、通道与坐标 ===");
    log("输入: assets/armor_red_frame0.png（讲义图 1 的显示缩放版）");
    log("");
    log("[图像基本信息]");
    log("rows(高) = " + std::to_string(img.rows) + ", cols(宽) = " + std::to_string(img.cols));
    log("channels = " + std::to_string(img.channels()));
    log("type = " + std::to_string(img.type()) + " , CV_8UC3 = " + std::to_string(CV_8UC3));
    log("depth = " + std::to_string(img.depth()) + " , CV_8U = " + std::to_string(CV_8U));
    log("step[0](每行字节数) = " + std::to_string(img.step[0]) + " = cols * channels = " +
        std::to_string(img.cols * img.channels()));
    log("像素数据总量 = rows * step = " +
        std::to_string(static_cast<long long>(img.rows) * img.step[0]) + " 字节");
    log("");
    log("[坐标与通道约定]");
    log("访问方式 img.at<cv::Vec3b>(y, x)：先写行 y，再写列 x");
    log("Vec3b[0]=B, [1]=G, [2]=R；OpenCV 默认彩色图是 BGR 而非 RGB");
    log("讲义主例是 1280x1024 原始帧，本图为其 0.8 倍缩放版，坐标换算 x'=0.8x, y'=0.8y");
    log("");
    log("[讲义图 2 的四个像素在本图上的实测值]");

    auto describe = [&](int x0, int y0) {
        const int x = cvRound(x0 * 0.8), y = cvRound(y0 * 0.8);
        if (x < 0 || x >= img.cols || y < 0 || y >= img.rows) return;
        const cv::Vec3b p = img.at<cv::Vec3b>(y, x);
        const int B = p[0], G = p[1], R = p[2];
        const int S = std::max(R - B, 0);
        log("原图(" + std::to_string(x0) + "," + std::to_string(y0) + ") -> 本图(" +
            std::to_string(x) + "," + std::to_string(y) + ")  B=" + std::to_string(B) +
            " G=" + std::to_string(G) + " R=" + std::to_string(R) +
            "  S=max(R-B,0)=" + std::to_string(S));
    };
    describe(912, 662);  // 灯条边缘
    describe(907, 662);  // 灯条中心
    describe(914, 662);  // 灯条外侧
    describe(900, 662);  // 背景

    const int row = cvRound(662 * 0.8);
    int best_x = -1, best_s = -1;
    for (int x = 550; x < 800 && x < img.cols; ++x) {
        const cv::Vec3b p = img.at<cv::Vec3b>(row, x);
        const int s = std::max(int(p[2]) - int(p[0]), 0);
        if (s > best_s) { best_s = s; best_x = x; }
    }
    log("");
    log("[第 " + std::to_string(row) + " 行扫描 x=550..799]");
    log("红色差分数最大的像素: x=" + std::to_string(best_x) + ", S=" + std::to_string(best_s));
    log("说明: 中心更亮但 S 更小，因为发白区域 B 通道同样很大；分数保留的是红蓝差异，不是亮度");

    cv::Rect roi(690, 490, 90, 85);
    roi &= cv::Rect(0, 0, img.cols, img.rows);
    cv::Mat patch = img(roi).clone();
    const int k = 8;
    cv::Mat zoom;
    cv::resize(patch, zoom, cv::Size(patch.cols * k, patch.rows * k), 0, 0, cv::INTER_NEAREST);
    for (int i = 0; i <= patch.cols; ++i)
        cv::line(zoom, cv::Point(i * k, 0), cv::Point(i * k, zoom.rows), cv::Scalar(70, 70, 70), 1);
    for (int j = 0; j <= patch.rows; ++j)
        cv::line(zoom, cv::Point(0, j * k), cv::Point(zoom.cols, j * k), cv::Scalar(70, 70, 70), 1);
    cv::imwrite(root + "/results/01_zoom.png", zoom);
    log("");
    log("已保存 results/01_zoom.png（右灯条附近 8 倍最近邻放大，网格为单个像素）");
    return 0;
}
