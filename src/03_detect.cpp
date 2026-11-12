#include <opencv2/opencv.hpp>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#ifndef PROJECT_ROOT
#define PROJECT_ROOT "."
#endif

static std::string fmt(double v, int p = 2) {
    std::ostringstream os;
    os << std::fixed << std::setprecision(p) << v;
    return os.str();
}

struct LightBar {
    cv::Point2f center, dir, top, bottom;
    double length = 0, width = 0, angle = 0, area = 0;
    std::vector<cv::Point2f> box;
};

// 四点按固定顺序连接后是否构成凸四边形，同时用鞋带公式算面积
static bool isConvexQuad(const std::vector<cv::Point2f>& p, double& area) {
    if (p.size() != 4) return false;
    int pos = 0, neg = 0;
    double area2 = 0;
    for (int i = 0; i < 4; ++i) {
        const cv::Point2f a = p[(i + 1) % 4] - p[i];
        const cv::Point2f b = p[(i + 2) % 4] - p[(i + 1) % 4];
        const double cr = a.x * b.y - a.y * b.x;
        if (cr > 1e-6) ++pos;
        else if (cr < -1e-6) ++neg;
        area2 += p[i].x * p[(i + 1) % 4].y - p[(i + 1) % 4].x * p[i].y;
    }
    area = std::fabs(area2) / 2.0;
    return (pos == 0 || neg == 0) && area >= 1.0;
}

int main() {
    const std::string root = PROJECT_ROOT;
    const std::string image_path = root + "/assets/armor_red_frame0.png";
    cv::Mat img = cv::imread(image_path, cv::IMREAD_COLOR);
    if (img.empty()) {
        std::cerr << "无法读取图像: " << image_path << std::endl;
        return 1;
    }

    std::ofstream out(root + "/results/03_detect.log");
    auto log = [&](const std::string& line) {
        std::cout << line << std::endl;
        out << line << std::endl;
    };

    // ---- 与练习 02 相同的分割流程 ----
    std::vector<cv::Mat> ch;
    cv::split(img, ch);
    const cv::Mat& B = ch[0];
    const cv::Mat& R = ch[2];
    cv::Mat diff16, score16, score8;
    cv::subtract(R, B, diff16, cv::noArray(), CV_16S);
    cv::Mat zero = cv::Mat::zeros(diff16.size(), diff16.type());
    cv::max(diff16, zero, score16);
    score16.convertTo(score8, CV_8U);
    cv::Mat scoreBin, rBin, mask, closed;
    cv::threshold(score8, scoreBin, 60, 255, cv::THRESH_BINARY);
    cv::threshold(R, rBin, 100, 255, cv::THRESH_BINARY);
    cv::bitwise_and(scoreBin, rBin, mask);
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::morphologyEx(mask, closed, cv::MORPH_CLOSE, kernel);

    log("=== 练习 03：轮廓、灯条几何与配对 ===");
    log("按讲义 2.7 的要求，日志保留每个被拒绝对象的具体数值，而不只显示最终候选数");
    log("");
    log("[阶段 1] 分割：S=max(R-B,0)>60 且 R>100，3x3 闭运算");
    log("");
    log("[阶段 2] 轮廓与灯条几何筛选");
    log("条件: 轮廓面积>=20, 长度 L>=6, W>0 且 L/W>=2, 偏离竖直方向<=35°");
    log("注意: 长轴端点由 c ± (L/2)u 得到，u 为统一朝下的长轴单位向量，不是两条长边的中点");

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(closed.clone(), contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    cv::Mat canvas = img.clone();
    std::vector<LightBar> bars;

    for (size_t i = 0; i < contours.size(); ++i) {
        const auto& c = contours[i];
        const double area = cv::contourArea(c);
        if (c.size() < 5) {
            log("轮廓 #" + std::to_string(i) + " 点数=" + std::to_string(c.size()) + " <5 -> 拒绝: 点数不足");
            continue;
        }
        cv::RotatedRect rr = cv::minAreaRect(c);
        cv::Point2f pts[4];
        rr.points(pts);
        const double len0 = cv::norm(pts[1] - pts[0]);
        const double len1 = cv::norm(pts[2] - pts[1]);
        const double L = std::max(len0, len1), W = std::min(len0, len1);
        cv::Point2f dir = (len0 >= len1) ? (pts[1] - pts[0]) : (pts[2] - pts[1]);
        const double n = cv::norm(dir);
        if (n < 1e-6) {
            log("轮廓 #" + std::to_string(i) + " -> 拒绝: 外接矩形退化");
            continue;
        }
        dir = cv::Point2f(dir.x / n, dir.y / n);
        if (dir.y < 0) dir = -dir;   // 统一朝向图像下方
        const double angle = std::atan2(std::fabs(dir.x), std::fabs(dir.y)) * 180.0 / CV_PI;

        std::string reason;
        if (area < 20) reason = "面积 " + fmt(area) + " < 20";
        else if (L < 6) reason = "长度 " + fmt(L) + " < 6";
        else if (!(W > 0) || L / W < 2) reason = "长宽比 " + fmt(W > 0 ? L / W : 0) + " < 2";
        else if (angle > 35) reason = "倾角 " + fmt(angle) + "° > 35°";

        log("轮廓 #" + std::to_string(i) +
            " 面积=" + fmt(area, 1) + " L=" + fmt(L, 1) + " W=" + fmt(W, 1) +
            " L/W=" + fmt(W > 0 ? L / W : 0) + " 倾角=" + fmt(angle, 1) + "°" +
            " 中心=(" + fmt(rr.center.x, 1) + "," + fmt(rr.center.y, 1) + ")");

        if (!reason.empty()) {
            log("  -> 拒绝: " + reason);
            continue;
        }
        log("  -> 接受为灯条候选");
        LightBar bar;
        bar.center = rr.center;
        bar.dir = dir;
        bar.length = L;
        bar.width = W;
        bar.angle = angle;
        bar.area = area;
        bar.top = cv::Point2f(rr.center.x - dir.x * L * 0.5, rr.center.y - dir.y * L * 0.5);
        bar.bottom = cv::Point2f(rr.center.x + dir.x * L * 0.5, rr.center.y + dir.y * L * 0.5);
        for (const auto& p : pts) bar.box.push_back(p);
        bars.push_back(bar);

        for (int e = 0; e < 4; ++e)
            cv::line(canvas, pts[e], pts[(e + 1) % 4], cv::Scalar(255, 200, 0), 1);
        cv::line(canvas, bar.top, bar.bottom, cv::Scalar(0, 255, 255), 1);
        cv::circle(canvas, bar.top, 3, cv::Scalar(0, 0, 255), -1);
        cv::circle(canvas, bar.bottom, 3, cv::Scalar(0, 0, 255), -1);
    }
    log("通过筛选的灯条数 = " + std::to_string(bars.size()));
    log("");
    log("[阶段 3] 灯条两两配对");
    log("条件: 长度比<=1.5, |dy|/Lbar<=0.35, d/Lbar∈[0.8,3.2], 长轴夹角<=15°, 四点凸且面积>=1");
    log("四点顺序: 0 左灯条上端 / 1 右灯条上端 / 2 右灯条下端 / 3 左灯条下端");

    int candidates = 0;
    for (size_t i = 0; i < bars.size(); ++i) {
        for (size_t j = i + 1; j < bars.size(); ++j) {
            const LightBar& a = bars[i];
            const LightBar& b = bars[j];
            const double lbar = (a.length + b.length) / 2.0;
            const double lr = std::max(a.length, b.length) / std::min(a.length, b.length);
            const double dyr = std::fabs(a.center.y - b.center.y) / lbar;
            const double dr = cv::norm(a.center - b.center) / lbar;
            double dot = std::max(-1.0, std::min(1.0, static_cast<double>(a.dir.dot(b.dir))));
            double ang = std::acos(dot) * 180.0 / CV_PI;
            ang = std::min(ang, 180.0 - ang);
            const LightBar* left = (a.center.x <= b.center.x) ? &a : &b;
            const LightBar* right = (a.center.x <= b.center.x) ? &b : &a;
            std::vector<cv::Point2f> quad{left->top, right->top, right->bottom, left->bottom};
            double quadArea = 0;
            const bool convex = isConvexQuad(quad, quadArea);

            log("组合 (" + std::to_string(i) + "," + std::to_string(j) + ")" +
                " 长度比=" + fmt(lr) + " |dy|/Lbar=" + fmt(dyr) +
                " d/Lbar=" + fmt(dr) + " 夹角=" + fmt(ang, 1) + "°" +
                " 凸=" + (convex ? "是" : "否") + " 四边形面积=" + fmt(quadArea, 1));

            std::string reason;
            if (lr > 1.5) reason = "长度比 " + fmt(lr) + " > 1.5";
            else if (dyr > 0.35) reason = "|dy|/Lbar " + fmt(dyr) + " > 0.35";
            else if (dr < 0.8 || dr > 3.2) reason = "d/Lbar " + fmt(dr) + " 不在 [0.8,3.2]";
            else if (ang > 15) reason = "夹角 " + fmt(ang, 1) + "° > 15°";
            else if (!convex) reason = "四点非凸或面积 " + fmt(quadArea, 1) + " < 1";

            if (!reason.empty()) {
                log("  -> 拒绝: " + reason);
                continue;
            }
            log("  -> 接受为装甲板候选 #" + std::to_string(candidates));
            ++candidates;

            std::vector<cv::Point> poly;
            for (const auto& p : quad) poly.push_back(cv::Point(cvRound(p.x), cvRound(p.y)));
            std::vector<std::vector<cv::Point>> pp{poly};
            cv::polylines(canvas, pp, true, cv::Scalar(0, 255, 0), 2);
            for (int k = 0; k < 4; ++k) {
                cv::circle(canvas, poly[k], 4, cv::Scalar(0, 255, 255), -1);
                cv::putText(canvas, std::to_string(k), poly[k] + cv::Point(8, -8),
                            cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(255, 255, 255), 2);
                log("  点 " + std::to_string(k) + ": (" + fmt(quad[k].x, 1) + ", " + fmt(quad[k].y, 1) + ")");
            }
        }
    }
    log("");
    log("最终装甲板候选数 = " + std::to_string(candidates));
    log("");
    log("[分层排查提示] 候选为 0 时按 输入 -> 分数 -> 掩膜 -> 连通结构 -> 灯条筛选 -> 配对 顺序看本日志，");
    log("先定位是哪一层的数值被拒绝，再有针对性地改该层参数，不要直接改颜色阈值。");

    cv::imwrite(root + "/results/03_result.png", canvas);

    // 四阶段拼接图，便于一眼看出信息在哪一步丢失
    cv::Mat a, b, c, d;
    const cv::Size half(512, 410);
    cv::resize(img, a, half);
    cv::resize(score8, b, half);
    cv::resize(mask, c, half);
    cv::resize(closed, d, half);
    cv::cvtColor(b, b, cv::COLOR_GRAY2BGR);
    cv::cvtColor(c, c, cv::COLOR_GRAY2BGR);
    cv::cvtColor(d, d, cv::COLOR_GRAY2BGR);
    cv::Mat top, bottom, grid;
    cv::hconcat(a, b, top);
    cv::hconcat(c, d, bottom);
    cv::vconcat(top, bottom, grid);
    cv::putText(grid, "input", cv::Point(10, 25), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 255), 2);
    cv::putText(grid, "color score S", cv::Point(522, 25), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 255), 2);
    cv::putText(grid, "mask", cv::Point(10, 435), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 255), 2);
    cv::putText(grid, "close", cv::Point(522, 435), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 255), 2);
    cv::imwrite(root + "/results/03_stages.png", grid);
    return 0;
}
